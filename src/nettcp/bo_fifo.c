#include "bo_fifo.h"
/* ----------------------------------------------------------------------------
 * Циклический (LinkedList)список хранящийся в массиве
 * @headItem	голова списка
 * @tail	хвост списка
 * @mem		указ на выдел память ч/з malloc
 * @count	колво item в списке
 * @nbyte	оставшиеся свободые байты в FIFO
 * @cur		
 */
struct BO_ITEM_FIFO {
	int n;
	char val[BO_FIFO_ITEM_VAL];
};

static struct FIFO {
	int itemN;
	struct BO_ITEM_FIFO *mem;
	int head; /*  индекс указ на голову */
	int tail; 
	int last;
	int count;
	int free;
} fifo = {0};

static struct TRANSACTION {
	/* [1] bo_insertFIFO was call [0] bo_insertFIFO wasn't call */
	int insert_status; 
	int tail;
	int count;
	int free;
} trans;

static int bo_get_fifo(unsigned char *buf, int bufSize);
static void bo_del_head();

static pthread_mutex_t fifo_mut = PTHREAD_MUTEX_INITIALIZER;

/* ----------------------------------------------------------------------------
 * @brief	Вывод очереди FIFO
 */
void bo_printFIFO() 
{
	pthread_mutex_lock(&fifo_mut);

	dbgout("FIFO: itemN[%d] free[%d]\n", fifo.itemN, fifo.free);
/*
	dbgout("=====\nFIFO HEAD[1..30]:\n");
	for(i = 0; i < 1; i++) {
		item_fifo = fifo.mem + fifo.head;
		dbgout("\n%d:[", i);
		for(j = 0; j < 30; j++) {
			dbgout("%c", item_fifo->val[j]);
		}
		dbgout("]\n=====\n");
	}
*/
	pthread_mutex_unlock(&fifo_mut);
}

void bo_printFIFO_test() 
{
	int i = 0, j = 0;
	struct BO_ITEM_FIFO *item_fifo;

	printf("FIFO: itemN[%d] free[%d] count[%d]\n", 
	fifo.itemN, 
	fifo.free,
	fifo.count);

	for(i = 0; i < fifo.itemN; i++) {
		item_fifo = fifo.mem + i;
		printf("\n%d:[", i);
		for(j = 0; j < 5; j++) {
			printf("%c", item_fifo->val[j]);
		}
		printf("]\n");
	}
}
/* ----------------------------------------------------------------------------
 * @brief	Создаем очередь 
 * @itemN	размер очередь в байтах
 * @return	[1] - OK; [-1] - ERROR
 */	
int bo_initFIFO(int itemN)	/*THREAD SAFE */
{
	int ans = -1;

	pthread_mutex_lock(&fifo_mut);
	
	fifo.mem = (struct BO_ITEM_FIFO *)
		malloc(sizeof(struct BO_ITEM_FIFO)*itemN);
	if(fifo.mem == NULL) goto exit;
	memset(fifo.mem, 0, sizeof(struct BO_ITEM_FIFO)*itemN);
	fifo.itemN	= itemN;
	fifo.head	= 0;
	fifo.tail	= 0;
	fifo.last	= itemN - 1;
	fifo.count	= 0;
	fifo.free	= itemN;
	trans.tail	= -1;
	trans.count	= -1;
	trans.free	= -1;
	ans		= 1;
	
	exit:
	pthread_mutex_unlock(&fifo_mut);
	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief	добавляем элем val размером size в очередь fifo  
 * @return	[1]  - OK; 
 *		[0]  - очередь заполнена
 *		[-1] - не коррект. значение size(больше макс допустимого 
 *			или меньше 1)
 */
int  bo_addFIFO(unsigned char *val, int size) /* THREAD SAFE */ 
{
	int ans = -1;
	struct BO_ITEM_FIFO *ptr = NULL;	
/*
	pthread_mutex_lock(&fifo_mut);
*/
	
	if(val == NULL) goto exit;
	if(size < 1)  goto exit;
	if(size > BO_FIFO_ITEM_VAL) goto exit;
	
	if(fifo.free != 0) {
		ptr = (fifo.mem + fifo.tail);
		memcpy(ptr->val, val, size);
		ptr->n = size;
		if(fifo.count == 0) fifo.head = fifo.tail;
		if(fifo.tail == fifo.last) fifo.tail = 0;
		else fifo.tail++;
		fifo.count++;
		fifo.free--;
		ans = 1;
	} else {
		ans = 0;
	}
	
	exit:
/*
	pthread_mutex_unlock(&fifo_mut);
*/
 	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief	сохраняем данные очереди для отката, если bo_addFIFO 
 *		вернет ошибку. После добавл данных необ-мо вызвать функцию
 *		bo_commitFIFO() | bo_cancelFIFO()
 * @return	[1] OK [-1] ERROR 
 */
int bo_insertFIFO()
{
	int ans = 1;
	pthread_mutex_lock(&fifo_mut);
	/* проверка пред-ая транзакция закончилась корректно */
	if(trans.insert_status == 0) {
		trans.insert_status	= 1;		
		trans.tail		= fifo.tail;
		trans.count		= fifo.count;
		trans.free		= fifo.free;
	} else {
		bo_log("bo_insertFIFO ERROR IN CODE run twice");
		ans = -1;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	call this function after bo_insertFIFO()
 */
void bo_commitFIFO()
{
	if(trans.insert_status == 1) {
		trans.tail		= -1;
		trans.count		= -1;
		trans.free		= -1;
		trans.insert_status	= 0;
		pthread_mutex_unlock(&fifo_mut);
	} else {
		bo_log("bo_commitFIFO() ERROR call before bo_insertFIFO()");
	}
}

/* ----------------------------------------------------------------------------
 * @brief	call this function after bo_cancelFIFO()
 */
void bo_cancelFIFO()
{
	if(trans.insert_status == 1) {
		fifo.tail		= trans.tail;
		fifo.count		= trans.count;
		fifo.free		= trans.free;
		bo_commitFIFO();
	} else {
		bo_log("bo_cancelFIFO() ERROR call before bo_insertFIFO()");
	}
}
/* ----------------------------------------------------------------------------
 * @brief	берем элемент из очереди
 * @buf		массив куда записываем данные
 * @bufSize     размер буфера
 * @return	[-1] - нет данных в очереди [N] - размер данных
 */
int bo_getFIFO(unsigned char *buf, int bufSize) /* THREAD SAFE */
{
	int ans = -1;
	pthread_mutex_lock(&fifo_mut);
	
	ans = bo_get_fifo(buf, bufSize); 
	
	pthread_mutex_unlock(&fifo_mut);
	return ans;
}

static int bo_get_fifo(unsigned char *buf, int bufSize)
{
	int ans = -1;
	struct BO_ITEM_FIFO *ptr = NULL;
	
	if(fifo.count != 0) {
		ptr = fifo.mem + fifo.head;
		if(bufSize >= ptr->n) {
			memcpy(buf, ptr->val, ptr->n);
			ans = ptr->n;
			/*
			fifo.count--;
			fifo.free++;
			if(fifo.head == fifo.last) fifo.head = 0;
			else fifo.head++;
			*/
		}
	}
	
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	берем голову удал голову
 * @return	[-1] - нет данных в очереди [N] - размер данных
 */
int bo_getFifoVal(unsigned char *buf, int bufSize) /*THREAD SAFE */
{
	int ans = -1;
	int timeLen = 50;
	char timeStr[50] = {0};
	if(fifo.count > 0) {
		pthread_mutex_lock(&fifo_mut);
		if(fifo.mem != NULL) {
			ans = bo_get_fifo(buf, bufSize);
			if(ans != -1) bo_del_head();

			bo_getTimeNow(timeStr, timeLen);
			timeStr[timeLen-1] = 0;
			fifo_log("\n[%s]>>>", timeStr);
			if(ans == -1) fifo_log("FIFO IS EMPTY\n");
			else {
				fifo_log("[");
				fifo_log10(buf, bufSize);
				fifo_log("]\n");
			}
		}
		pthread_mutex_unlock(&fifo_mut);
	} else {
		sleep(1);
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief		Делаем головой очереди следующий элемент
 */
void bo_delHead() /*THREAD SAFE*/
{
	pthread_mutex_lock(&fifo_mut); 
	bo_del_head();
	pthread_mutex_unlock(&fifo_mut); 
}

static void bo_del_head()
{
	if(fifo.count != 0) {
		memset(fifo.mem + fifo.head, 0, sizeof(struct BO_ITEM_FIFO));
		fifo.count--;
		fifo.free++;
		if(fifo.head == fifo.last) fifo.head = 0;
		else fifo.head++;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	удаляем хвост списка
 */
void bo_fifo_delLastAdd()
{
	pthread_mutex_lock(&fifo_mut);
	if(fifo.count != 0) {
		fifo.count--;
		fifo.free++;
		if(fifo.tail == 0) fifo.tail = fifo.last;
		else fifo.tail--;
	}
	pthread_mutex_unlock(&fifo_mut);
}
/* ----------------------------------------------------------------------------
 * @brief	кол-во свободных item
 */
int bo_getFree()
{
	return fifo.free;
}

/* ----------------------------------------------------------------------------
 * @brief	колво хран эелементов
 */
int bo_getCount()
{
	return fifo.count;
}

/* ----------------------------------------------------------------------------
 * @brief	прибираем за собой
 */
void bo_delFIFO()
{
	pthread_mutex_lock(&fifo_mut);

	free(fifo.mem);
	fifo.mem = NULL;
	fifo.head = 0;
	fifo.tail = 0;
	fifo.last = 0;
	fifo.count = 0;
	fifo.free = 0;
	
	pthread_mutex_unlock(&fifo_mut); 
}

/* 0x42 */