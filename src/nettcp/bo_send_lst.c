#include "bo_send_lst.h"

static int bo_add	(char *ip);
static int get_lazy_sock();
static void bo_del_item_sock_lst(int i);
static int getFreeInd();
static int port_serv;

/* ----------------------------------------------------------------------------
 * @brief	создаем список сокетов
 * @size	размер списка
 * @return	[-1] ERROR [1] OK
*/
int bo_init_sock_lst(int size, int port)
{
	int ans = -1;
	int i = -1;
	int error = -1;
	
	port_serv = port;
	
	sock_lst.size = size;
	sock_lst.head = -1;
	sock_lst.tail = -1;
	sock_lst.n    = 0;

	sock_lst.prev  = NULL;
	sock_lst.next  = NULL;
	sock_lst.free  = NULL;

	sock_lst.arr = (struct BO_ITEM_SOCK_LST *)
		malloc(sizeof(struct BO_ITEM_SOCK_LST)*size);
	
	if(sock_lst.arr == NULL) goto error_label;

	sock_lst.next = (int *)malloc(size*sizeof(int));
	if(sock_lst.next == NULL) goto error_label;

	sock_lst.prev = (int *)malloc(size*sizeof(int));
	if(sock_lst.prev == NULL) goto error_label;

	sock_lst.free = (int *)malloc(size*sizeof(int));
	if(sock_lst.free == NULL) goto error_label;

	/* создание стека своб ячеек для связ списка */
	for(i = 0; i < size; i++) {
		*(sock_lst.free + i) = i + 1;
		if(i == (size - 1)) {
			*(sock_lst.free + i) = -1;
		}
	}
	*(sock_lst.prev)	= -1;
	*(sock_lst.next)	= -1;
	sock_lst.arr->sock	= -1;
		
	ans = 1;
	
	if(error == 1) {
error_label:
		if(sock_lst.arr   != NULL) free(sock_lst.arr);
		if(sock_lst.prev  != NULL) free(sock_lst.prev);
		if(sock_lst.next  != NULL) free(sock_lst.next);
		if(sock_lst.free  != NULL) free(sock_lst.free);
	}
	return ans;
}


void bo_del_sock_lst()
{
	/* Сокеты должны быть закрыты !!! до удаления*/
	if(sock_lst.arr   != NULL) free(sock_lst.arr);
	if(sock_lst.prev  != NULL) free(sock_lst.prev);
	if(sock_lst.next  != NULL) free(sock_lst.next);
	if(sock_lst.free  != NULL) free(sock_lst.free);

	sock_lst.arr  = NULL;
	sock_lst.prev = NULL;
	sock_lst.next = NULL;
	sock_lst.free = NULL;		
}

/* ----------------------------------------------------------------------------
 * @brief	возвращает индекс своб позиции в списке
 *		Стек строится на массиве. В ячейке хран-ся индекс след своб 
 *		ячейки. Первая ячейка хранит вершину стека.
 *		|1|2|3|-1| <- значения
 *		|0|1|2| 3| <- индекс массива
 * @return	[-1] - список полный  [>-1] - индекс свободной ячейки
 */
static int getFreeInd()
{
	int ans = -1;
	ans = *(sock_lst.free); /* вершина стека */
	/* если стек не полный(-1 признак полн стека) */
	if(ans != -1) {
		/* вершиной стека делаем след элемент */
		*(sock_lst.free) = *(sock_lst.free + ans);
		*(sock_lst.free + ans) = -1;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	добавление элемента в список
 * @ip		must C string with last element '\0'
 * @return	[1] - OK [-1] ERROR
 */
int bo_add_sock_lst(char *ip)
{
	int i		= -1;
	int ans		= -1;
	int exec	= -1;
	
	i = *sock_lst.free;
	printf("bo_add_sock_lst free index[%d]\n", i);
	if(i != -1) {
		exec = bo_add(ip);
		if(exec == -1) {
			bo_log("bo_add_sock_lst() ip[%s] ERROR", ip);
			ans = -1;
			goto exit;
		}
		ans = 1;
	} else {
		/* find lazy sock del it and add new */
		i = get_lazy_sock();
		bo_del_item_sock_lst(i);
		exec = bo_add(ip);
		if(exec == -1) {	
			bo_log("bo_add_sock_lst() ip[%s] ERROR", ip);
			ans = -1;
			goto exit;
		} 
		ans = 1;
	}
	exit:
	return ans;
}

/* [-1] ERROR [1] OK */
static int bo_add(char *ip)
{
	int i		= -1;
	int sock	= -1;
	int temp	= -1;
	int ans		= -1;
	struct BO_ITEM_SOCK_LST *item = NULL;
	

	sock = bo_setConnect(ip, port_serv);
	if(sock == -1) {
		bo_log("bo_send_lst.c->bo_add->bo_setConnect ip[%s] errno[%s]",
			ip,
			strerror(errno));
		goto exit;
	}

	i = getFreeInd();

	item = sock_lst.arr + i;
	item->sock = sock;
	item->rate = 0;
	memset(item->ip, 0, 16);
	memcpy(item->ip, ip, strlen(ip));

	if(sock_lst.n == 0) {
		/* добавление первого элемента */
		*(sock_lst.prev + i) = -1;
		*(sock_lst.next + i) = -1;
		sock_lst.head = i;
	} else {
		temp = sock_lst.tail; 
		*(sock_lst.prev + i)	 = temp;
		*(sock_lst.next + i)	 = -1;
		*(sock_lst.next + temp)	 = i;
	}
	sock_lst.tail = i;
	sock_lst.n++;
	ans = 1;
	
	exit:
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	закрытие сокета -> удаление злемента
 * @i		индекс в массиве
 */
static void bo_del_item_sock_lst(int i)
{
	int buf = -1;
	int pr = -1, nx = -1; 
	struct BO_ITEM_SOCK_LST *item = NULL;
	
	item = sock_lst.arr + i;
	close(item->sock);
	item->sock = 0;
	
	item->rate = -1;
	memset(item->ip, 0, 16);
	
	/* добавлеям индекс в стек свободных ячеек */
	buf = *sock_lst.free;
	*sock_lst.free = i;
	*(sock_lst.free + i) = buf;
	
	/* удаляем элемент из связ списка */
	pr = *(sock_lst.prev + i);
	*(sock_lst.prev + i) = -1;
	nx = *(sock_lst.next + i);
	*(sock_lst.next + i) = -1;
	
	if(pr != -1) *(sock_lst.next + pr) = nx;
	else sock_lst.head = nx;
	if(nx != -1) *(sock_lst.prev + nx) = pr;
	else sock_lst.tail = pr;
	sock_lst.n--;
}

/* ----------------------------------------------------------------------------
 * @brief	возвращает сокет у которого рейтинг минимальный
 * @return	[0 - 254] index from sock_lst.arr array 
 */
static int get_lazy_sock()
{
	int index = -1, i = -1, min_rate = 1000000;
	struct BO_ITEM_SOCK_LST *item = NULL; 
		
	i = sock_lst.head;
	index = i;
	while(i != -1) {
		item = sock_lst.arr + i;
		if(item->rate < min_rate) {
			min_rate = item->rate;	
			index = i;
		}
		i = *(sock_lst.next + i);
	}
	
	return index;
}

/* ----------------------------------------------------------------------------
 * @return	[-1] no sock [>0]  sock 
 */
int bo_get_sock_by_ip(char *ip)
{
	int ans = -1, i = -1;
	struct BO_ITEM_SOCK_LST *item = NULL; 

	i = sock_lst.head;
	while(i != -1) {
		item = sock_lst.arr + i;
		printf("bo_get_sock_by_ip -> ip[%s]", item->ip);
		if(strstr(ip, item->ip)) {
			ans = item->sock;
			item->rate++;
			goto exit;
		}
		i = *(sock_lst.next + i);
	}
	
	exit:
	return ans;
}

void bo_print_sock_lst() 
{
	int i = 0;
	int size = 0;
	struct BO_ITEM_SOCK_LST *item = NULL;
	
	size = sock_lst.size;
	size = 3;
	printf("size[%d] n[%d]\nind :", size, sock_lst.n);
	for(i = 0; i < size; i++) {
		printf("[%d] ", i);
	}
	printf("\nsock:");
	for(i = 0; i < size; i++) {
		item = sock_lst.arr + i;
		printf("[%d] ", item->sock);
	}
	printf("\nprev:");
	
	for(i = 0; i < size; i++) {
		printf("[%d] ", *(sock_lst.prev + i));
	}
	printf("\nnext:");
	
	for(i = 0; i < size; i++) {
		printf("[%d] ", *(sock_lst.next + i));
	}
	printf("\nfree:");
	
	for(i = 0; i < size; i++) {
		printf("[%d] ", *(sock_lst.free + i));
	}
	printf("\n");
}
