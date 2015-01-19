/*
 *		Moxa-slave
 *                  Сервис SNMP
 *                  Сервер FIFO
 *                  Send FIFO
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
#include "bo_fifo_out.h"
#include "bo_net_master_core.h"
#include "bo_snmp_mng.h"


/**
 * snmp_serv - Сервис SNMP.
 * @arg:  Параметры для потока.
 *
 * Мониторинг состояния магистрали.
 */
void *snmp_serv(void *arg)
{
	struct snmp_thread_arg *targ = (struct snmp_thread_arg *)arg;
	
	bo_snmp_main(targ->ip, targ->n);
	
	bo_log("snmp_serv: exit");
	pthread_exit(0);
}

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
 * send_fifo - Посылка данных на чужой узел в стек FIFO.
 * @arg:  Параметры для потока.
 *
 *
 */
void *send_fifo(void *arg)
{
	struct sfifo_thread_arg *targ = (struct sfifo_thread_arg *)arg;
	char id[9];
	int i;
	int ans;
	int np;

	while (1) {

		memset(sfifo.ip, 0, 16);
		
		/** Формирование индекса для кадра посылаемого на
		 * чужой узел (защита от дублирования пакетов) */
		fifo_idx++;
		if (fifo_idx == FIFO_IDX_MAX) fifo_idx = 1;
		
		sprintf(id, "%08d", (fifo_idx & 0xffffffff));
		
		/** Подготовка буфера для FIFO */
		for (i=0; i<8; i++)
			sfifo.buf[i] = id[i];

		sfifo.ln = bo_get_fifo_out(sfifo.buf+8, BUF485_SZ, sfifo.ip);
		
		if (sfifo.ln > 0) {
			
			np = 0;
			ans = 0;
			
			sfifo.ln += 8;
			
			bo_log("bo_sendDataFIFO ip= [%s] ln= [%d]-->", sfifo.ip, sfifo.ln);
			
			while (ans != 1) {
				ans = bo_sendDataFIFO(sfifo.ip,
						      targ->port,
						      (char *)sfifo.buf,
						      sfifo.ln);
				if (np >= 10) break;
				if (ans != 1) {
					np++;
					usleep(200000);
				}
			}
			
			bo_log("bo_sendDataFIFO finish np= [%d]-->", np);
			
			if (ans != 1)
				bo_log("send_fifo(): bo_sendDataFIFO(): ERROR");
		} else
			/** Нет данных в очереди */
			usleep(200000);
	}
	
	bo_log("send_fifo: exit");
	pthread_exit(0);
}

/**
 * logSendSock_connect - Создает сокет и подключается к серверу логов
 *                       на мастере.
 * @arg:  Параметры для потока.
 *
 */
void *logSendSock_connect(void *arg)
{
	struct log_thread_arg *targ = (struct log_thread_arg *)arg;
	int exec = -1;
	int sock_temp;

	while (1) {
		sock_temp = -1;
		sock_temp = bo_setConnect(targ->logSend_ip,
					  targ->logSend_port);
		if (sock_temp < 0) {
			bo_log("logSendSock_connect(): %s",
				"logSend_sock=bo_setConnect() ERROR");
			sleep(10);
			continue;
		}
		
		pthread_mutex_lock(&mx_sendSocket);

		logSend_sock = sock_temp;

		pthread_mutex_unlock(&mx_sendSocket);
		
		bo_log("logSend_sock(): IN socket[%d] ok", logSend_sock);
		
		while (1) {
			/** Организовать функцию проверки сокета на
			 * наличие коннекта !!! */
			pthread_mutex_lock(&mx_sendSocket);
			
			exec = bo_chkSock(logSend_sock);
			if (exec == -1) {
				bo_closeSocket(logSend_sock);
				bo_log("logSend_sock(): IN socket[%d] close",
				       logSend_sock);
				
				pthread_mutex_unlock(&mx_sendSocket);
				sleep(10);
				break;
			}

			pthread_mutex_unlock(&mx_sendSocket);
			sleep(10);
		}
	}
	
	bo_log("logSendSock_connect: exit");
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
	struct timeval tval;
	
	int ans;
	fd_set r_set;
	int exec = -1;

	tval.tv_sec = 0;
	tval.tv_usec = 100000;
	
	while (1) {
		if (rtSend_sock != -1) {
			rtRecv_sock = bo_setConnect(targ->ip, targ->port);
			if (rtRecv_sock < 0) {
				sleep(10);
				continue;
			}
		} else {
			sleep(10);
			continue;
		}

		p.sock = rtRecv_sock;
		p.route_tab = rtg;
		p.buf = rtBuf;
		p.bufSize = BO_MAX_TAB_BUF;

		bo_log("rtbl_recv(): OUT socket[%d] ok", rtRecv_sock);
		
		while (1) {
			FD_ZERO(&r_set);
			FD_SET(rtRecv_sock,  &r_set);

			exec = select(rtRecv_sock+1, &r_set, NULL, NULL, &tval);

			if(exec == -1) {
				bo_log("rtbl_recv(): select errno[%s]",
				       strerror(errno));
				break;
			} else if(exec == 0) {
				if (rtSend_sock == -1) {
					
					bo_log("rtbl_recv(): rtSend_sock == -1");
					break;
				}
				
			} else {
				/* если событие произошло */
				pthread_mutex_lock(&mx_rtg);

				ans = bo_master_core(&p);

				pthread_mutex_unlock(&mx_rtg);
				if (ans < 0) {
					bo_log("rtbl_recv(): %s",
					       "bo_master_core ERROR");
					break;
				}
			}
		}
		
		bo_closeSocket(rtRecv_sock);
		bo_log("rtbl_recv(): OUT socket[%d] close", rtRecv_sock);
		
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
	int sock_temp;
	
	while (1) {
		sock_temp = -1;
		sock_temp = bo_setConnect(targ->ip, targ->port);
		if (sock_temp < 0) {
			bo_log("rtbl_send: bo_setConnect ERROR");
			sleep(10);
			continue;
		}
		pthread_mutex_lock(&mx_rtl);

		rtSend_sock = sock_temp;

		/** Отправка локальной таблицы мастеру */
		send_rtbl(targ->host_ip);

		pthread_mutex_unlock(&mx_rtl);
		
		bo_log("rtbl_send(): IN socket[%d] ok", rtSend_sock);
		
		
		while (1) {
			/** Организовать функцию проверки сокета на
			 * наличие коннекта !!! */
			pthread_mutex_lock(&mx_rtl);
			
			exec = bo_chkSock(rtSend_sock);

			if (exec == -1) {
				bo_closeSocket(rtSend_sock);
				bo_log("rtbl_send(): IN socket[%d] close",
				       rtSend_sock);
				rtSend_sock = -1;
				pthread_mutex_unlock(&mx_rtl);
				sleep(10);
				break;
			}

			/** Отправка локальной таблицы мастеру */
			send_rtbl(targ->host_ip);

			pthread_mutex_unlock(&mx_rtl);
			sleep(30);
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
				write (1, "WDT refresh ---------------\n", 28);
#endif
				mxwdg_refresh(wdt_fd);
			} else {
#ifdef __PRINT__
				write (1, "----------- WDT not refresh\n", 28);
#endif
			}
		}

#endif  /** defined (__MOXA_TARGET__) && defined (__WDT__) */

		inc_cron_life(targ->lifeFile);
	}
	
	bo_log("wdt: exit");
	pthread_exit(0);
}

