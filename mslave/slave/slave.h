/*
 *		Moxa-slave
 *
 * Version:	@(#)slave.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _SLAVE_H
#define _SLAVE_H


#include <pthread.h>

#include "oht.h"
#include "ocfg.h"
#include "ocs.h"


/** Кол-во (макс.) устройств на каждой шине RS485 */
#define DST_BUF_SZ 32


struct fifo_thread_arg {
	int port;         /** Номер порта FIFO сервера */
	int qu_len;       /** Очередь коннектов */
	int len;          /** Размер очереди */
};

#ifdef MOXA_TARGET
struct wdt_thread_arg {
	int tsec;         /** Цикл WatchDog sec */
	int tusec;        /** Цикл WatchDog usec */
	int wdt_en;       /** Разрешение работы WatchDog */
};
#endif

struct chan_thread_arg {
	int tsec;         /** Базовый таймер sec */
	int tusec;        /** Базовый таймер usec */
	int tscan;        /** Таймер сканирования устройств на сети RS485 */
	int tout;         /** Таймаут приема кадра на сети RS485 */
#ifdef MOXA_TARGET
	int wdt_en;       /** Разрешение работы WatchDog */
#endif
	int nretries;     /** Число попыток передачи кадра по сети RS485 */
	int port;         /** Номер порта RS485 */
	int src;          /** Адрес устройства в сети RS485 */
	int ch1_enable;   /** Разрешение на работу канала RS485 (порт 1) */
	int ch2_enable;   /** Разрешение на работу канала RS485 (порт 2) */
	int dst_beg;      /** Диапазон адресов для поиска устройств на
			   * сети RS485 */
	int dst_end;
	char *ip;         /** IP адрес узла */
	int fifo_port;    /** Номер порта FIFO сервера */
	char *logSend_ip; /** IP адрес LOG сервера */
	int logSend_port; /** Номер порта LOG сервера */
	int cdaId;        /** Идентификатор запроса 'AccessGranted' */
	int cdaDest;      /** Идентификатор подсистемы ЛС */
	int cdnsId;       /** Идентификатор запроса 'GetNetworkStatus' */
	int cdnsDest;     /** Идентификатор подсистемы ЛС */
	int cdquLogId;    /** Идентификатор запроса 'GetLog' */
	int cdquDest;     /** Идентификатор подсистемы ЛС */
};

struct rt_thread_arg {
	char *ip;
	int port;
};

struct sta {
	int state;
	pthread_mutex_t mx;
};
struct sta psvdata_ready;

struct thr_dst_buf {
	int buf[DST_BUF_SZ];      /** Буфер адресов усройств RS485 */
	pthread_mutex_t mx;       /** Защита доступа к буферу */
	int rpos;                 /** Позиция чтения из буфера */
	int wpos;                 /** Позиция записи в буфер */
};
struct thr_dst_buf dstBuf;
struct thr_dst_buf dst2Buf;

pthread_attr_t pattr;

pthread_mutex_t	mx_psv;

struct thr_rx_buf rxBuf;
struct thr_tx_buf txBuf;
struct thr_rx_buf rx2Buf;
struct thr_tx_buf tx2Buf;

/** Условная переменная для синхронизации обмена данными между
 * активными и пассивными устройствами */
pthread_cond_t psvdata;

pthread_mutex_t	mx_rtl;
pthread_mutex_t	mx_rtg;

TOHT *rtl;  /** Таблица маршрутов собственный узел */
TOHT *rtg;  /** Таблица маршрутов все узлы */

pthread_mutex_t	mx_sendSocket;

int rtSend_sock;
int rtRecv_sock;

int logSend_sock;

/** Идентификатор кадров отправляемых через FIFO */
unsigned int fifo_idx;

unsigned char *rtBuf;


#ifdef MOXA_TARGET

#define WDT_LIVING 0x11
#define WDT_CHAN1  0x01
#define WDT_CHAN2  0x10

int wdt_fd;  /** WatchDog файл дескриптор */
struct sta wdt_life;

void init_thrWdtlife(struct sta *st);
void destroy_thrWdtlife(struct sta *st);
void set_wdtlife(struct sta *st, int data);
void put_wdtlife(struct sta *st, int data);
int get_wdtlife(struct sta *st);

#endif

void gen_moxa_default_cfg(char *cfile);

void init_thrState(struct sta *st);
void destroy_thrState(struct sta *st);
void put_state(struct sta *st, int data);
int get_state(struct sta *st);

void init_thrDstBuf(struct thr_dst_buf *b);
void destroy_thrDstBuf(struct thr_dst_buf *b);
int test_bufDst(struct thr_dst_buf *b, int data);
int remf_bufDst(struct thr_dst_buf *b, int data);
int put_bufDst(struct thr_dst_buf *b, int data);
int get_bufDst(struct thr_dst_buf *b);

void put_rtbl(struct chan_thread_arg *targ, struct thr_dst_buf *b, int dst);
void remf_rtbl(struct chan_thread_arg *targ, struct thr_dst_buf *b, int dst);
void putLog();
void sendFIFO(int fifo_port, char *key);

void prepare_cadr_scan(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst);
void prepare_cadr(struct thr_tx_buf *b, char *buf, int ln);
void prepare_cadr_act(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst);
void prepare_cadr_actNetStat(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst);
void prepare_cadr_quLog(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst);


#endif	/* _SLAVE_H */

