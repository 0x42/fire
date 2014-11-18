#include "bo_cycle_arr.h"
/* ----------------------------------------------------------------------------
 * Циклический массив
 * @headItem	голова списка
 * @tail	хвост списка
 * @mem		указ на выдел память ч/з malloc
 * @count	колво item в списке
 */
struct bo_cycle_arr_item {
	int n;
	unsigned char log[BO_ARR_ITEM_VAL];
};

struct bo_cycle_arr {
	int itemN;
	struct bo_cycle_arr_item *mem;
	int tail; 
};

/* ----------------------------------------------------------------------------
 * @brief	созд цикл массива
 * @return	[NULL] - ERROR; [*ptr] - указатель на массив
 */
struct bo_cycle_arr *bo_initCycleArr(int n)
{
	struct bo_cycle_arr *cl = NULL;
	
	cl = (struct bo_cycle_arr *)malloc(sizeof(struct bo_cycle_arr));
	if(cl == NULL) goto exit;
	
	cl->mem = (struct bo_cycle_arr_item *)
		malloc( sizeof(struct bo_cycle_arr_item)*n );
	if(cl->mem == NULL) {
		free(cl);
		cl = NULL;
		goto exit;
	}
	
	memset(cl->mem, 0, sizeof(struct bo_cycle_arr_item)*n);

	cl->itemN = n;
	cl->tail = 0;
	exit:
	return cl;
}

/* ----------------------------------------------------------------------------
 * @brief	добавляем элемент в хвост
 * @return	[-1] - ERROR [1] - OK
 */
int bo_cycle_arr_add(struct bo_cycle_arr *arr, unsigned char *value, int n)
{
	int ans = -1;
	struct bo_cycle_arr_item *item = NULL;
	item = (arr->mem + arr->tail);
	if(n < BO_ARR_ITEM_VAL) {
		memset(item->log, 0, BO_ARR_ITEM_VAL);
		memcpy(item->log, value, n);
		item->n = n;
		arr->tail++;
		if(arr->tail == arr->itemN) arr->tail = 0;
		ans = 1;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	копируем строку из массива в value по индексу i
 * @return	[>0] размер данных скоп в value
 *		[0] нет данных по этому индексу
 *		[-1] ERROR плохой индекс
 */
int bo_cycle_arr_get(struct bo_cycle_arr *arr, unsigned char *value, int i)
{
	int ans = -1;
	struct bo_cycle_arr_item *item = NULL;
	
	if(i >= arr->itemN) goto exit;
	
	item = (arr->mem + i);
	ans = item->n;
	if(item->n == 0) goto exit;
	
	memcpy(value, item->log, item->n);
	exit:
	return ans;
}

void bo_cycle_arr_del(struct bo_cycle_arr *arr)
{
	if(arr != NULL) {
		if(arr->mem != NULL) free(arr->mem);
		arr->mem = NULL;
		free(arr);
	}
}
/* 0x42 */