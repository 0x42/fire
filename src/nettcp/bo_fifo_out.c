#include "bo_fifo_out.h"
/* ----------------------------------------------------------------------------
 * Циклический список на основе bo_fifo.h
 * @ip
 * @val  - данные для отправки |LEN|DATA|LEN|DATA|...|LEN|DATA LEN = 2bytes
 * @size - размер данных
 */
struct BO_ITEM_FIFO_OUT {
	char ip[16];
	char val[BO_FIFO_ITEM_VAL];
	int  size;
};

static struct FIFO {
	int itemN;
	struct BO_ITEM_FIFO_OUT *mem;
	int head; /*  индекс указ на голову */
	int tail; 
	int last;
	int count;
	int free;
} fifo = {0};

static int bo_getPos_fifo_out(char *ip);
static pthread_mutex_t fifo_out_mut = PTHREAD_MUTEX_INITIALIZER;

/* ----------------------------------------------------------------------------
 * @brief	Создаем очередь 
 * @itemN	размер очереди
 * @return	[1] - OK; [-1] - ERROR
 */	
int bo_init_fifo_out(int itemN)	/*THREAD SAFE */
{
	int ans = -1;

	pthread_mutex_lock(&fifo_out_mut);
	
	fifo.mem = (struct BO_ITEM_FIFO_OUT *)
		malloc(sizeof(struct BO_ITEM_FIFO_OUT)*itemN);
	if(fifo.mem == NULL) goto exit;
	memset(fifo.mem, 0, sizeof(struct BO_ITEM_FIFO_OUT)*itemN);
	fifo.itemN = itemN;
	fifo.head = 0;
	fifo.tail = 0;
	fifo.last = itemN - 1;
	fifo.count = 0;
	fifo.free = itemN;
	ans = 1;
	
	exit:
	pthread_mutex_unlock(&fifo_out_mut);
	return ans;
}

/* ----------------------------------------------------------------------------
 * @breif	добавляем данные по IP
 * @ip		type string
 * @return	[1]  - OK; 
 *		[0]  - очередь заполнена
 *		[-1] - не коррект. значение size(больше макс допустимого 
 *			или меньше 1)
 */
int bo_add_fifo_out(unsigned char *val, int size, char *ip) /* THREAD SAFE */
{
	int ans = -1, pos = -1, temp = 0, i = 0;
	unsigned char len[2] = {0};
	struct BO_ITEM_FIFO_OUT *ptr = NULL;	

	pthread_mutex_lock(&fifo_out_mut);
	if(size >= BO_FIFO_ITEM_VAL) goto exit;
	if(size < 1) goto exit;
	if(val == NULL) goto exit;
	
	pos = bo_getPos_fifo_out(ip);
	if(pos != -1) {
		ptr = fifo.mem + pos;
		temp = ptr->size + size + 4;
		if(temp > BO_FIFO_ITEM_VAL) {
			bo_log("FIFO OUT fifo.free[%d] fifo.count[%d]",
				fifo.free, fifo.count);
			if(fifo.count > 0) {
				i = fifo.head;
				while(i != fifo.tail ) {
					ptr = fifo.mem + i;
					bo_log("FIFO OUT IP[%s] SIZE[%d]", 
						ptr->ip, ptr->size);
					if(i == fifo.last) i = 0;
					else i++;
				}
			}
			ans = 0;
			goto exit;
		}
		/* |LEN| */
		boIntToChar(size, len);
		memcpy(ptr->val + ptr->size, len, 2);
		ptr->size += 2;
		/* |DATA| */
		memcpy(ptr->val + ptr->size, val, size);
		ptr->size += size;
		ans = 1;
	} else {
		if(fifo.free == 0) { 
			bo_log("FIFO OUT fifo.free = 0 fifo.count[%d]", fifo.count);
			if(fifo.count > 0) {
				i = fifo.head;
				while(i != fifo.tail ) {
					ptr = fifo.mem + i;
					bo_log("FIFO OUT IP[%s] SIZE[%d]", 
						ptr->ip, ptr->size);
					if(i == fifo.last) i = 0;
					else i++;
				}
			}
			ans = 0;
			goto exit;
		}
		ptr = fifo.mem + fifo.tail;
		/* LEN */
		boIntToChar(size, len);
		memcpy(ptr->val, len, 2);
		ptr->size += 2;
		/* DATA*/
		memcpy(ptr->val + ptr->size, val, size);
		ptr->size += size;
		/* IP */
		memcpy(ptr->ip, ip, strlen(ip));
		if(fifo.count == 0) fifo.head = fifo.tail;
		
		if(fifo.tail == fifo.last) fifo.tail = 0;
		else fifo.tail++;
		
		fifo.count++;
		fifo.free--;
		ans = 1;
	}
	exit:
	pthread_mutex_unlock(&fifo_out_mut);
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	берем данные из очереди FIFO OUT
 * @ip		размер должен быть = 16
 * @return	[-1] нет данных в очереди [N] размер данных записаных в буфер
 */
int bo_get_fifo_out(unsigned char *buf, int bufSize, char *ip)
{
	int ans = -1;
	struct BO_ITEM_FIFO_OUT *ptr = NULL;
	pthread_mutex_lock(&fifo_out_mut);
	if(fifo.count != 0) {
		ptr = fifo.mem + fifo.head;
		if(bufSize >= ptr->size) {
			memcpy(buf, ptr->val, ptr->size);
			memcpy(ip, ptr->ip, strlen(ptr->ip));
			ans = ptr->size;
			
			memset(ptr->ip, 0, 16);
			ptr->size = 0;
			
			fifo.count--;
			fifo.free++;
			if(fifo.head == fifo.last) fifo.head = 0;
			else fifo.head++;
		}
	}
	pthread_mutex_unlock(&fifo_out_mut);
	return ans;
}

/* ----------------------------------------------------------------------------
 * @return	[-1]not found [N] индекс BO_ITEM_FIFO_OUT c @ip 
 */
static int bo_getPos_fifo_out(char *ip)
{
	int ans = -1, i = 0, n = 0;
	struct BO_ITEM_FIFO_OUT *ptr = NULL;
	if(ip == NULL) goto exit;
	
	if(fifo.count != 0) {
		i = fifo.head;
		while(i != fifo.tail ) {
			ptr = fifo.mem + i;
			if(strstr(ptr->ip, ip)) {
				ans = i; 
				goto exit;
			}
			if(i == fifo.last) i = 0;
			else i++;
		}
	}
	exit:
	return ans;
}