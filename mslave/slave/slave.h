/*
 *		Moxa-slave
 *
 * Version:	@(#)slave.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _SLAVE_H
#define _SLAVE_H


#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "oht.h"
#include "ocfg.h"
#include "ocs.h"


/** Кол-во (макс.) устройств на каждой шине RS485 */
#define DST_BUF_SZ 32

/** Структура данных для потока fifo_serv() */
struct fifo_thread_arg {
	int port;         /** Номер порта FIFO сервера */
	int qu_len;       /** Очередь коннектов */
	int len;          /** Размер очереди */
};

/** Структура данных для потока logSendSock_connect() */
struct log_thread_arg {
	char *logSend_ip; /** IP адрес LOG сервера */
	int logSend_port; /** Номер порта LOG сервера */
};

/** Структура данных для потока wdt() */
struct wdt_thread_arg {
	int tsec;         /** Цикл WatchDog sec */
	int tusec;        /** Цикл WatchDog usec */
	int wdt_en;       /** Разрешение работы WatchDog */
	char *lifeFile;   /** Файл для контроля жизни программы через CRON */
};

/** Структура данных для потоков chan1(), chan2() */
struct chan_thread_arg {
	int tsec;         /** Базовый таймер sec */
	int tusec;        /** Базовый таймер usec */
	int tscan;        /** Таймер сканирования устройств на сети RS485 */
	int tout;         /** Таймаут приема кадра на сети RS485 */
	int wdt_en;       /** Разрешение работы WatchDog */
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
	int logMaxLines;  /** Максимальное количество строк лога */
	int cdaId;        /** Идентификатор запроса 'AccessGranted' */
	int cdaDest;      /** Идентификатор подсистемы ЛС */
	int cdnsId;       /** Идентификатор запроса 'GetNetworkStatus' */
	int cdnsDest;     /** Идентификатор подсистемы ЛС */
	int cdquLogId;    /** Идентификатор запроса 'GetLog' */
	int cdquDest;     /** Идентификатор подсистемы ЛС */
};

/** Структура данных для потоков rtbl_recv(), rtbl_send() */
struct rt_thread_arg {
	char *ip;
	int port;
};

/** Структура данных для хранения адресов усройств сети RS485 */
struct thr_dst_buf {
	int buf[DST_BUF_SZ];      /** Буфер адресов усройств RS485 */
	pthread_mutex_t mx;       /** Защита доступа к буферу */
	int rpos;                 /** Позиция чтения из буфера */
	int wpos;                 /** Позиция записи в буфер */
};
struct thr_dst_buf dstBuf;        /** Адреса усройств RS485 (порт 1) */
struct thr_dst_buf dst2Buf;       /** Адреса усройств RS485 (порт 2) */

/** Структуры для работы с приемными и передающими буферами каналов
 * RS485 (порт 1, 2) */
struct thr_rx_buf rxBuf;
struct thr_tx_buf txBuf;
struct thr_rx_buf rx2Buf;
struct thr_tx_buf tx2Buf;

/** Структура для работы с переменными состояния */
struct sta {
	int state;
	pthread_mutex_t mx;
};

/** Синхронизация обмена данными между активными и пассивными устройствами */
pthread_cond_t psvdata;  /** Условная переменная */
pthread_mutex_t	mx_psv;  /** защита */
struct sta psvdata_ready;  /** Переменная состояния */

/** Атрибуты функционирования нитей PTHREAD */
pthread_attr_t pattr;

pthread_mutex_t	mx_rtl;  /** Защита локальной таблицы маршрутов */
pthread_mutex_t	mx_rtg;  /** Защита глобальной таблицы маршрутов */

TOHT *rtl;  /** Таблица маршрутов собственный узел */
TOHT *rtg;  /** Таблица маршрутов все узлы */

int rtSend_sock;  /** Сокет для отправки локальной таблицы
		   * маршрутов контроллеру master */
int rtRecv_sock;  /** Сокет для получения глобальной таблицы
		   * маршрутов от контроллера master */

int logSend_sock;  /** Сокет для работы с логами */

pthread_mutex_t	mx_sendSocket;  /** Защита ресурса logSend_sock */


/** Идентификатор кадров отправляемых через FIFO */
unsigned int fifo_idx;
#define FIFO_IDX_MAX 100000

/** Буфер для глобальной таблицы маршрутов загружаемой с контроллера master */
unsigned char *rtBuf;


#if defined (__MOXA_TARGET__) && defined (__WDT__)

/** Значения для флага wdt_life в контрольных точках */
#define WDT_CHAN1  0x01  /** chan1() */
#define WDT_CHAN2  0x10  /** chan2() */
#define WDT_LIVING 0x11  /** WDT_CHAN1 || WDT_CHAN2 */

int wdt_fd;  /** WatchDog файл дескриптор */
struct sta wdt_life;  /** Флаг для определения живучести системы */

void init_thrWdtlife(struct sta *st);
void destroy_thrWdtlife(struct sta *st);
void set_wdtlife(struct sta *st, int data);
void put_wdtlife(struct sta *st, int data);
int get_wdtlife(struct sta *st);

#endif  /** defined (__MOXA_TARGET__) && defined (__WDT__) */

void gen_moxa_default_cfg(char *cfile);
/* void gen_moxa_cron_life(char *cfile); */
void inc_cron_life(char *lifile);

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

void prepare_cadr(struct thr_tx_buf *b, char *buf, int ln);
void prepare_cadr_scan(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst);
void prepare_cadr_act(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst);
void prepare_cadr_actNetStat(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst);
void prepare_cadr_quLog(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst);


/** ------------------------- Потоки ------------------------- */
void *wdt(void *arg);        /** WatchDog */

void *chan1(void *arg);      /** (канал 1 - активные устройства) */
void *chan2(void *arg);      /** (канал 2 - пассивные устройства) */
void *rtbl_recv(void *arg);  /** Получение глобальной таблицы
			      * маршрутов от мастера */
void *rtbl_send(void *arg);  /** Отправка локальной таблицы маршрутов
			      * мастеру */
void *fifo_serv(void *arg);  /** Сервер FIFO */
void *logSendSock_connect(void *arg);  /** Сервер LOG */


#endif	/* _SLAVE_H */

