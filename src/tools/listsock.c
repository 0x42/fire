#include "listsock.h"

/* ----------------------------------------------------------------------------
 * @brief	создаем список сокетов
 * @size	размер массива
 * @return      [NULL] - error [>0] - список
*/

struct bo_llsock *bo_crtLLSock(int size)
{
	int i = 0;
	int error = -1;
	struct bo_llsock *list = NULL;
	list = (struct bo_llsock *)malloc(sizeof(struct bo_llsock));
	if(list != NULL) {
		list->size = size;
		list->head = -1;
		list->tail = -1;
		list->n    = 0;
		
		list->val   = NULL;
		list->prev  = NULL;
		list->next  = NULL;
		list->free  = NULL;
		
		list->val = (struct bo_sock *)malloc(size*sizeof(struct bo_sock));
		if(list->val == NULL) goto error_label;
		
		list->next = (int *)malloc(size*sizeof(int));
		if(list->next == NULL) goto error_label;
				
		list->prev = (int *)malloc(size*sizeof(int));
		if(list->prev == NULL) goto error_label;
		
		list->free = (int *)malloc(size*sizeof(int));
		if(list->free == NULL) goto error_label;
		
		/* создание стека своб ячеек для связ списка*/
		for(i = 0; i < size; i++) {
			*(list->free + i) = i + 1;
			if(i == (size - 1)) {
				*(list->free + i) = -1;
			}
		}
		*(list->prev) = -1;
		*(list->next) = -1;
		list->val->sock = -1;
		
	}
	
	if(error == 1) {
error_label:
		if(list->val   != NULL) free(list->val);
		if(list->prev  != NULL) free(list->prev);
		if(list->next  != NULL) free(list->next);
		if(list->free  != NULL) free(list->free);
		if(list        != NULL) free(list);
	}
	return list; 
}

void bo_del_lsock(struct bo_llsock *llist)
{
	if(llist != NULL) {
		/* Сокеты должны быть закрыты !!! до удаления*/
		free(llist->val);
		free(llist->prev);
		free(llist->next);
		free(llist->free);
		
		llist->val	 = NULL;
		llist->prev	 = NULL;
		llist->next	 = NULL;
		llist->free	 = NULL;
		
		free(llist);
		llist		 = NULL;
		
	}
}

/* ----------------------------------------------------------------------------
 * @brief	возвращает индекс своб позиции в списке
 *		Стек строится на массиве. В ячейке хран-ся индекс след своб 
 *		ячейки. Первая ячейка хранит вершину стека.
 *		|1|2|3|-1| <- значения
 *		|0|1|2| 3| <- индекс массива
 * @return	[-1] - список полный  [>-1] - индекс свободной ячейки
 */
static int getFreeInd(struct bo_llsock *llist)
{
	int ans = -1;
	ans = *(llist->free); /* вершина стека */
	/* если стек не полный(-1 признак полн стека) */
	if(ans != -1) {
		/* вершиной стека делаем след элемент */
		*(llist->free) = *(llist->free + ans);
		*(llist->free + ans) = -1;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	добавление элемента в список
 * @return	[1] - OK [-1] Список полный
 */
int bo_addll(struct bo_llsock *llist, int sock)
{
	int i = -1;
	int t = -1;
	int ans = -1;
	struct bo_sock *bs = NULL;
	
	i = getFreeInd(llist);
	if(i != -1) {
		bs = llist->val + i;
		bs->sock = sock;
		if(llist->n == 0) {
			/* добавление первого элемента */
			*(llist->prev + i) = -1;
			*(llist->next + i) = -1;
			llist->head = i;
		} else {
			t = llist->tail; 
			*(llist->prev + i) = t;
			*(llist->next + i) = -1;
			*(llist->next + t) = i;
		}
		llist->tail = i;
		llist->n++;
		ans = 1;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	удаление злемента
 * @i		индекс в массиве
 */
void bo_del_val(struct bo_llsock *llist, int i)
{
	int buf = -1;
	int pr = -1, nx = -1; 
	/* добавлеям индекс в стек свободных ячеек */
	buf = *llist->free;
	*llist->free = i;
	*(llist->free + i) = buf;
	
	/* удаляем элемент из связ списка */
	pr = *(llist->prev + i);
	*(llist->prev + i) = -1;
	nx = *(llist->next + i);
	*(llist->next + i) = -1;
	
	if(pr != -1) *(llist->next + pr) = nx;
	else llist->head = nx;
	if(nx != -1) *(llist->prev + nx) = pr;
	else llist->tail = pr;
	llist->n--;
}

/* ----------------------------------------------------------------------------
 * @brief	получить элемент
 * @val		через указатель вернем результат
 * @i		индекс по которому берем значение из списка
 * @return	[-1] список пуст [>0] индекс след элемента
 */
int bo_get_val(struct bo_llsock *llist, struct bo_sock **val, int i)
{
	int ans = -1;
	*val = NULL;
	if(i > -1) {
		*val = llist->val + i;
		ans = *(llist->next + i);
	}
	return ans;
}


/* ----------------------------------------------------------------------------
 * @brief	возв голову списка
 */
int bo_get_head(struct bo_llsock *llist) 
{
	return llist->head;
}

/* ----------------------------------------------------------------------------
 * @brief	возв размер списка
*/
int bo_get_len(struct bo_llsock *llist)
{
	return llist->n;
}

/* ----------------------------------------------------------------------------
* @brief	удал item в списке по значению 
*/
void bo_del_bysock(struct bo_llsock *llist, int sock)
{
	int i = -1, exec = -1;
	struct bo_sock *val = NULL;
	
	i = bo_get_head(llist);
	while(i != -1) {
		exec = bo_get_val(llist, &val, i);
		if(val->sock == sock) bo_del_val(llist, i);
		i = exec;
	}
}

void bo_print_list(struct bo_llsock *llist) 
{
	int i = 0;
	int size = 0;
	struct bo_sock *v;
	size = llist->size;
	printf("size[%d] n[%d]\nind :", size, llist->n);
	for(i = 0; i < size; i++) {
		printf("[%d] ", i);
	}
	printf("\nval :");
	for(i = 0; i < size; i++) {
		v = llist->val + i;
		printf("[%d] ", v->sock);
	}
	printf("\nprev:");
	
	for(i = 0; i < size; i++) {
		printf("[%d] ", *(llist->prev + i));
	}
	printf("\nnext:");
	
	for(i = 0; i < size; i++) {
		printf("[%d] ", *(llist->next + i));
	}
	printf("\nfree:");
	
	for(i = 0; i < size; i++) {
		printf("[%d] ", *(llist->free + i));
	}
	printf("\n");
}

/* 0x42 */