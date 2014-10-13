#ifndef BO_FIFO_H
#define	BO_FIFO_H
#define BO_FIFO_ITEM_VAL 1200
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


int bo_initFIFO(int size);

int bo_addFIFO(unsigned char *obj, int size);

int bo_getFIFO(unsigned char *buf, int bufSize);

void bo_delHead();

int bo_getFree();

void bo_delFIFO();
#endif	
/* 0x42 */

