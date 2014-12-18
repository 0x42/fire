/*
 *		Moxa-slave
 *                  Сервер FIFO
 *                  Сервер LOG коннект
 *                  Таблица маршрутов (получение)
 *                  Таблица маршрутов (отправка)
 *                  WatchDog
 *
 * Version:	@(#)a_threads.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */


#include "slave.h"
#include "bologging.h"
#include "bo_net.h"
#include "bo_fifo.h"
#include "bo_net_master_core.h"

/* extern void bo_fifo_thrmode(int port, int queue_len, int fifo_len); */

/**
 * fifo_serv - Сервер FIFO.
 * @arg:  Параметры для потока.
 *
 * Сервер организует стек запросов FIFO.
 */
void *fifo_serv(void *arg)
{
	struct fifo_thread_arg *targ = (struct fifo_thread_arg *)arg;

	while (1) {
		bo_fifo_thrmode(targ->port, targ->qu_len, targ->len);
		bo_log("fifo_serv: restarted");
		sleep(1);
	}
	
	bo_log("fifo_serv: exit");
	pthread_exit(0);
}

/**
 * logSendSock_connect - Создает сокет и подключается к серверу логов на мастере.
 * @arg:  Параметры для потока.
 *
 */
void *logSendSock_connect(void *arg)
{
	struct log_thread_arg *targ = (struct log_thread_arg *)arg;

	while (1) {
		logSend_sock = bo_setConnect(targ->logSend_ip, targ->logSend_port);
		if (logSend_sock < 0) {
			bo_log("logSendSock_connect(): logSend_sock=bo_setConnect() ERROR");
			sleep(10);
			continue;
		}
		break;
	}
	
	bo_log("logSend_sock: socket ok");
	
	pthread_exit(0);
}

/**
 * rtbl_recv - Создание сокета и подключения к мастеру для получения от него
 *             глобальной таблицы маршрутов.
 * @arg:  Параметры для потока.
 */
void *rtbl_recv(void *arg)
{
	struct rt_thread_arg *targ = (struct rt_thread_arg *)arg;
	struct paramThr p;
	int ans;
	fd_set r_set;
	int exec = -1;
	
	while (1) {
		rtRecv_sock = bo_setConnect(targ->ip, targ->port);		
		if (rtRecv_sock < 0) {
			sleep(10);
			continue;
		}

		p.sock = rtRecv_sock;
		p.route_tab = rtg;
		p.buf = rtBuf;
		p.bufSize = BO_MAX_TAB_BUF;

		bo_log("rtbl_recv(): socket ok");
		
		while (1) {
			FD_ZERO(&r_set);
			FD_SET(rtRecv_sock,  &r_set);

			exec = select(rtRecv_sock+1, &r_set, NULL, NULL, NULL);

			if(exec == -1) {
				bo_log("rtbl_recv(): select errno[%s]",
				       strerror(errno));
				break;
			} else if(exec == 0) {
				bo_log("rtbl_recv(): ???");
				break;
			} else {
				/* если событие произошло */
				pthread_mutex_lock(&mx_rtg);
				ans = bo_master_core(&p);
				pthread_mutex_unlock(&mx_rtg);
				if (ans < 0) {
					bo_log("rtbl_recv(): bo_master_core ERROR");
					break;
				}
			}
		}
		
		bo_closeSocket(rtRecv_sock);
	}
	
	bo_log("rtbl_recv: exit");
	pthread_exit(0);
}

/**
 * rtbl_send - Создание сокета и подключения к мастеру для отправки ему
 *             локальной таблицы маршрутов.
 * @arg:  Параметры для потока.
 */
void *rtbl_send(void *arg)
{
	struct rt_thread_arg *targ = (struct rt_thread_arg *)arg;
	int exec = -1;
	
	while (1) {
		pthread_mutex_lock(&mx_rtl);

		rtSend_sock = bo_setConnect(targ->ip, targ->port);
		
		if (rtSend_sock < 0) {
			bo_log("rtbl_send: bo_setConnect ERROR");
			pthread_mutex_unlock(&mx_rtl);
			sleep(10);
			continue;
		}
		pthread_mutex_unlock(&mx_rtl);
		
		bo_log("rtbl_send(): socket ok");
		
		while (1) {
			/** Организовать функцию проверки сокета на
			 * наличие коннекта !!! */
			pthread_mutex_lock(&mx_rtl);
			
			exec = bo_chkSock(rtSend_sock);
			if (exec == -1) {
				pthread_mutex_unlock(&mx_rtl);
				break;
				
			}
			pthread_mutex_unlock(&mx_rtl);
			
			sleep(10);
		}
	}
	
	bo_log("rtbl_send: exit");
	pthread_exit(0);
}


/**
 * wdt - WatchDog.
 * @arg:  Параметры для потока.
 *
 * Поток выполняет следующие действия:
 * 1) Организует таймер;
 * 2) По таймеру проверяет флаг wdt_life на предмет жива система или нет.
 *    Если система жива, то производится обновление таймера Watchdog.
 *    В противном случае, через заданное время произойдет перезагрузка системы.
 */
void *wdt(void *arg)
{
	struct wdt_thread_arg *targ = (struct wdt_thread_arg *)arg;
	struct timeval t;
	
	while (1) {
		t.tv_sec = targ->tsec;
		t.tv_usec = targ->tusec;
		select(0, NULL, NULL, NULL, &t);
		
#if defined (__MOXA_TARGET__) && defined (__WDT__)

		if (targ->wdt_en) {
			if (get_wdtlife(&wdt_life) == WDT_LIVING) {
				/** Если контрольные участки кода пройдены,
				 * подтвердить работоспособность системы */
				set_wdtlife(&wdt_life, 0);
				/* refresh the timer */
#ifdef __PRINT__
				write (1, "WDT refresh -----------------\n", 30);
#endif
				mxwdg_refresh(wdt_fd);
			} else {
#ifdef __PRINT__
				write (1, "------------- WDT not refresh\n", 30);
#endif
			}
		}

#endif  /** defined (__MOXA_TARGET__) && defined (__WDT__) */

		inc_cron_life(targ->lifeFile);
	}
	
	bo_log("wdt: exit");
	pthread_exit(0);
}

