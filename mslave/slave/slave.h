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
#include "bo_fifo.h"


/** Кол-во (макс.) устройств на каждой шине RS485 */
#define DST_BUF_SZ 256

/** Кол-во (макс.) IP адресов для контроля магистрали (SNMP) */
#define SNMP_IP_MAX 64

/** Структура данных для потока snmp_serv() */
struct snmp_thread_arg {
	char *ip[SNMP_IP_MAX];  /** Массив IP адресов для мониторинга
			         * состояния магистрали */
	int n;                  /** Размер массива */
};

/** Структура данных для потока fifo_serv() */
struct fifo_thread_arg {
	int port;         /** Номер порта FIFO сервера */
	int qu_len;       /** Очередь коннектов */
	int len;          /** Размер очереди */
};

/** Структура данных для потока send_fifo() */
struct sfifo_thread_arg {
	int port;         /** Номер порта FIFO сервера */
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
	int tscan;        /** Таймер сканирования устройств на сети RS485 */
	int tout;         /** Таймаут приема кадра на сети RS485 */
	unsigned int utxdel;  /** Коэффициент для задержки на передачу
			       * по сети RS485 */
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
	int snmp_n;       /** Размер массива IP адресов для мониторинга
			   * состояния магистрали */
	int snmp_uso;     /** Адрес УСО для передачи состояния магистрали */
	int logMaxLines;  /** Максимальное количество строк лога */
	int cdDest;       /** Идентификатор подсистемы ЛС */
	int cdaId;        /** Идентификатор запроса 'AccessGranted' */
	int cdnsId;       /** Идентификатор запроса 'GetNetRS485Status' */
	int cdquLogId;    /** Идентификатор запроса 'GetLog' */
	int cdmsId;       /** Идентификатор запроса 'GetNetworkStatus' */
};

/** Структура данных для потоков rtbl_recv(), rtbl_send() */
struct rt_thread_arg {
	char *ip;
	int port;
	char *host_ip;    /** IP адрес узла */
};

/** Структура данных для хранения адресов усройств сети RS485 */
struct thr_dst_buf {
	int buf[DST_BUF_SZ];      /** Буфер адресов усройств RS485 */
	int fbuf[DST_BUF_SZ];     /** Буфер флажков усройств RS485
				   * работа с FIFO */
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
pthread_cond_t psvdata;       /** Условная переменная */
pthread_cond_t psvAnsdata;    /** Условная переменная */
pthread_mutex_t	mx_psv;       /** защита */
struct sta psvdata_ready;     /** Переменная состояния */
struct sta psvAnsdata_ready;     /** Переменная состояния */

/** Синхронизация обмена данными при ответе пассивного устройства на
 * запрос активному устройству FIFO */
pthread_cond_t actFIFOdata;    /** Условная переменная */
pthread_mutex_t	mx_actFIFO;    /** защита */
struct sta actFIFOdata_ready;  /** Переменная состояния */

/** Структура данных для организации обмена данными между узлами */
struct thr_fifo_buf {
	unsigned char buf[BUF485_SZ];    /** Буфер данных для передачи через FIFO */
	char ip[16];            /** Адрес IP узла */
	int ln;                 /** Длина данных */
};
struct thr_fifo_buf sfifo;


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

unsigned char getFifo_buf[BO_FIFO_ITEM_VAL];
int getFifo_ans;

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

void send_rtbl(char *ip);
void put_rtbl(struct chan_thread_arg *targ, struct thr_dst_buf *b, int dst);
void remf_rtbl(struct chan_thread_arg *targ, struct thr_dst_buf *b, int dst);
void putLog();

void prepareFIFO(struct thr_rx_buf *rb, char *key, int dst);

void prepare_cadr(struct thr_tx_buf *b, char *buf, int ln);
void prepare_cadr_scan(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst);

int tx(struct chan_thread_arg *targ, struct thr_tx_buf *b, char *msg);
int rx(struct chan_thread_arg *targ, struct thr_rx_buf *b, int tout, char *msg);

int scan(struct chan_thread_arg *targ,
	 struct thr_tx_buf *tb,
	 struct thr_rx_buf *rb,
	 struct thr_dst_buf *db,
	 int dst,
	 char *msg);

extern void bo_fifo_thrmode(int port, int queue_len, int fifo_len);

/** ------------------------- Потоки ------------------------- */
void *wdt(void *arg);        /** WatchDog */

void *chan1(void *arg);      /** (канал 1 - активные устройства) */
void *chan2(void *arg);      /** (канал 2 - пассивные устройства) */
void *rtbl_recv(void *arg);  /** Получение глобальной таблицы
			      * маршрутов от мастера */
void *rtbl_send(void *arg);  /** Отправка локальной таблицы маршрутов
			      * мастеру */
void *fifo_serv(void *arg);  /** Сервер FIFO */
void *send_fifo(void *arg);  /** Стек FIFO */
void *logSendSock_connect(void *arg);  /** Сервер LOG */
void *snmp_serv(void *arg);  /** Сервер SNMP */


#endif	/* _SLAVE_H */

