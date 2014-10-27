#include "bo_fifo.h"
/* ----------------------------------------------------------------------------
 * Циклический(односвязанный) список хранящийся в массиве
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
	int head; /*  индекс указ на голову*/
	int tail; 
	int last;
	int count;
	int free;
} fifo = {0};
/* ----------------------------------------------------------------------------
 * @brief	Вывод очереди FIFO
 */
void bo_printFIFO() 
{
	int i = 0, j = 0;
	struct BO_ITEM_FIFO *item_fifo;
	printf("\n=========================================================\n");
	printf("FIFO:\n itemN[%d]\nhead[%d]\ntail[%d]\nlast[%d]\ncount[%d]\nfree[%d]\n",
		   fifo.itemN, fifo.head, fifo.tail, fifo.last, fifo.count, fifo.free);
	printf("FIFO item size[%d]\n", BO_FIFO_ITEM_VAL);
	printf("  ---------------------------------------------------------\n");
	
	for(i = 0; i < fifo.itemN; i++) {
		item_fifo = fifo.mem + i;
		printf("\n=====\ni=%d\n=====\n", i);
		for(j = 0; j < BO_FIFO_ITEM_VAL; j++) {
			if(j == 16) printf("\n");
			printf("0x%02x ", item_fifo->val[j]);
		}
	}
	printf("\n=========================================================\n");
}
/* ----------------------------------------------------------------------------
 * @brief	Создаем очередь 
 * @itemN	размер очередь в байтах
 * @return	[1] - OK; [-1] - ERROR
 */
int bo_initFIFO(int itemN)
{
	int ans = -1;
	fifo.mem = (struct BO_ITEM_FIFO *)
		malloc(sizeof(struct BO_ITEM_FIFO)*itemN);
	if(fifo.mem == NULL) goto exit;
	memset(fifo.mem, 0, sizeof(struct BO_ITEM_FIFO)*itemN);
	fifo.itemN = itemN;
	fifo.head = 0;
	fifo.tail = 0;
	fifo.last = itemN - 1;
	fifo.count = 0;
	fifo.free = itemN;
	ans = 1;
	exit:
	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief	добавляем элем val размером size в очередь fifo  
 * @return	[1]  - OK; 
 *		[0]  - очередь заполнена
 *		[-1] - не коррект. значение size(больше макс допустимого 
 *			или меньше 1)
 */
int  bo_addFIFO(unsigned char *val, int size) 
{
	int ans = -1;
	struct BO_ITEM_FIFO *ptr = NULL;
	if(val == NULL) goto exit;
	if(size < 1) goto exit;
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
	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief	берем элемент из очереди
 * @buf		массив куда записываем данные
 * @bufSize     размер буфера
 * @return	[-1] - нет данных в очереди [N] - размер данных
 */
int bo_getFIFO(unsigned char *buf, int bufSize)
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
 * @brief		Делаем головой очереди следующий элемент
 */
void bo_delHead()
{
	if(fifo.count != 0) {
		memset(fifo.mem+fifo.head, 0, sizeof(struct BO_ITEM_FIFO));
		fifo.count--;
		fifo.free++;
		if(fifo.head == fifo.last) fifo.head = 0;
		else fifo.head++;
	}
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
	free(fifo.mem);
	fifo.mem = NULL;
	fifo.head = 0;
	fifo.tail = 0;
	fifo.last = 0;
	fifo.count = 0;
	fifo.free = 0;
}