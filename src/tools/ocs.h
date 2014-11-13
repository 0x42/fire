/*
 *		Обработка кадра данных (канал RS485).
 *
 * Version:	@(#)ocs.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _OCS_H
#define _OCS_H


#include <pthread.h>


/** Размер буферов приемника и передатчика RS485
#define BUF485_SZ 1032 */
#define BUF485_SZ 1200


struct thr_rx_buf {
	char buf[BUF485_SZ];     /** Буфер приемника RS485 */
	pthread_mutex_t mx;      /** Защита доступа к буферу */
	int rpos;                /** Позиция чтения из буфера */
	int wpos;                /** Позиция записи в буфер */
	int fl;                  /** Состояния при приеме кадра:
				  * 0- ожидание данных,
				  * 1- получили Sync,
				  * 2- начало данных,
				  * 3- получили ESC(0xDB),
				  * 4- конец приема данных,
				  * 5- кадр поврежден,
				  * 6- таймаут */
	pthread_cond_t empty;    /** Буфер пустой */
	pthread_cond_t full;     /** Буфер заполнен */
};

struct thr_tx_buf {
	char buf[BUF485_SZ];     /** Буфер передатчика RS485 */
	pthread_mutex_t mx;      /** Защита доступа к буферу */
	int rpos;                /** Позиция чтения из буфера */
	int wpos;                /** Позиция записи в буфер */
	pthread_cond_t empty;    /** Буфер пустой */
	pthread_cond_t full;     /** Буфер заполнен */
};


void init_thrRxBuf(struct thr_rx_buf *b);
void destroy_thrRxBuf(struct thr_rx_buf *b);
char get_rxBuf(struct thr_rx_buf *b);
void put_rxBuf(struct thr_rx_buf *b, char data);
int get_rxFl(struct thr_rx_buf *b);
void put_rxFl(struct thr_rx_buf *b, int fl);

void init_thrTxBuf(struct thr_tx_buf *b);
void destroy_thrTxBuf(struct thr_tx_buf *b);
char get_txBuf(struct thr_tx_buf *b);
void put_txBuf(struct thr_tx_buf *b, char data);

int read_byte(struct thr_rx_buf *b, char data, int fl);
int reader(struct thr_rx_buf *b, char *buf, int port, int ptout);
int prepare_buf_tx(struct thr_tx_buf *b, char *buf);
int writer(struct thr_tx_buf *b, char *buf, int port);


#endif	/* _OCS_H */

