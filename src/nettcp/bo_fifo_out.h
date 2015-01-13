#ifndef BO_FIFO_OUT_H
#define	BO_FIFO_OUT_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>

#include "bo_fifo.h"

/* ----------------------------------------------------------------------------
 * @brief	Создаем очередь 
 * @itemN	размер очереди
 * @return	[1] - OK; [-1] - ERROR
 */	
int bo_init_fifo_out(int itemN);	/*THREAD SAFE */

/* ----------------------------------------------------------------------------
 * @breif	добавляем данные по IP
 * @ip		type string
 * @return	[1]  - OK; 
 *		[0]  - очередь заполнена
 *		[-1] - не коррект. значение size(больше макс допустимого 
 *			или меньше 1)
 */
int bo_add_fifo_out(unsigned char *val, int size, char *ip); /* THREAD SAFE */

/* ----------------------------------------------------------------------------
 * @brief	берем данные из очереди FIFO OUT
 * @ip		размер должен быть = 16
 * @return	[-1] нет данных в очереди [N] размер данных записаных в буфер
 */
int bo_get_fifo_out(unsigned char *buf, int bufSize, char *ip); /* THREAD SAFE */

#endif	/* BO_FIFO_OUT_H */

