#ifndef BO_FIFO_H
#define	BO_FIFO_H
#define BO_FIFO_ITEM_VAL 1200
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../tools/dbgout.h"

void bo_printFIFO(); /* THREAD SAFE */ 

int bo_initFIFO(int size); /* THREAD SAFE */ 

int bo_addFIFO(unsigned char *obj, int size); /* THREAD SAFE */ 

int bo_getFifoVal(unsigned char *buf, int bufSize); /* THREAD SAFE */ 

int bo_getFIFO(unsigned char *buf, int bufSize); /* THREAD SAFE */ 

void bo_delHead(); /* THREAD SAFE */ 

int bo_getFree(); /* THREAD SAFE */ 

void bo_delFIFO(); /* THREAD SAFE */ 

#endif	
/* 0x42 */

