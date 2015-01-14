#ifndef BO_FIFO_H
#define	BO_FIFO_H
#define BO_FIFO_ITEM_VAL 1200
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include "../tools/dbgout.h"
#include "../log/bologging.h"

void bo_printFIFO(); /* THREAD SAFE */ 

void bo_printFIFO_test();

int bo_initFIFO(int size); /* THREAD SAFE */ 
/* -------------------------------------------------------------------------- */
/* THREAD SAFE */
int bo_addFIFO(unsigned char *obj, int size);  
int bo_insertFIFO();
 /* @brief	call this function after bo_insertFIFO()  */
void bo_commitFIFO();
 /* @brief	call this function after bo_cancelFIFO()
  *             if bo_addFIFO() return ERROR|FULL
  *   */
void bo_cancelFIFO();
/* -------------------------------------------------------------------------- */

void bo_fifo_delLastAdd(); /* THREAD SAFE */

int bo_getFifoVal(unsigned char *buf, int bufSize); /* THREAD SAFE */ 

int bo_getFIFO(unsigned char *buf, int bufSize); /* THREAD SAFE */ 

void bo_delHead(); /* THREAD SAFE */ 

int bo_getFree(); /* THREAD SAFE */ 

int bo_getCount();

void bo_delFIFO(); /* THREAD SAFE */ 

#endif	
/* 0x42 */

