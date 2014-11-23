#ifndef BO_CYCLE_LIST_H
#define	BO_CYCLE_LIST_H
#define BO_ARR_ITEM_VAL 1200
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct bo_cycle_arr *bo_cycle_arr_init(int n);

int bo_cycle_arr_add(struct bo_cycle_arr *arr, unsigned char *value, int n);

int bo_cycle_arr_get(struct bo_cycle_arr *arr, unsigned char *value, int i);

void bo_cycle_arr_del(struct bo_cycle_arr *arr);

void bo_cycle_arr_print(struct bo_cycle_arr *arr);

#endif	/* BO_CYCLE_LIST_H */


/* 0x42 */