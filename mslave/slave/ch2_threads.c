
#include "ch_threads.h"
#include "slave.h"
#include "ort.h"
#include "bologging.h"
#include "bo_net.h"
#include "bo_fifo.h"
#include "bo_fifo.c"
#include "bo_net_master_core.h"


int trx2(struct chan_thread_arg *targ)
{
	char tbuf[BUF485_SZ];  /** Буфер передатчика RS485 */
	char buf[BUF485_SZ];  /** Буфер приемника RS485 */
	int res;

	/** Tx */
	res = writer(&tx2Buf, tbuf, targ->port);	
	if (res < 0) return -1;

	/** Rx */
	put_rxFl(&rx2Buf, RX_WAIT);  /** Состояние приема - 'ожидание данных' */
	rx2Buf.wpos = 0;  /** Позиция записи в начало буфера приемника */
	res = 1;
	
	while (res) {
		res = reader(&rx2Buf, buf, targ->port, targ->tout);
		if (res < 0) return -1;
	}
	
	return 0;
}

int scan2(struct chan_thread_arg *targ, int dst)
{
	char a[4];
	int res;

	prepare_cadr_scan(targ, &tx2Buf, dst);
	
	/** Данные для передачи подготовлены */
	res = trx2(targ);
	if (res < 0) return -1;
	
	if ((get_rxFl(&rx2Buf) >= RX_DATA_READY) &&
	    ((rx2Buf.buf[1] & 0xFF) == dst)) {
		switch (get_rxFl(&rx2Buf)) {
		case RX_DATA_READY:
			put_rtbl(targ, &dst2Buf, dst);
			
			memset(a, 0, 3);
			sprintf(a, "%d", dst);
			write(1, a, 4);
			
			break;
		case RX_ERROR:
			/** Ошибка кадра */
			bo_log("scan2(): Cadr Error !");
			break;
		case RX_TIMEOUT:
			/** Текущее устройство не отвечает,
			 * вычеркиваем его из списка. */
			bo_log("scan2(): timeout dst= %d", dst);
			remf_rtbl(targ, &dst2Buf, dst);
			
			write(1, ",", 1);
			break;
		default:
			bo_log("scan2(): state ??? fl= %d", get_rxFl(&rx2Buf));
			break;
		}
	} else {
		write(1, "_", 1);
	}

	return 0;
}


int send_dataToPassive(struct chan_thread_arg *targ)
{	
	int nretr = targ->nretries;
	unsigned char buf[BO_FIFO_ITEM_VAL];
	int bufSize = BO_FIFO_ITEM_VAL;
	int dst;
	int ans;
	int res;
	
	if (get_state(&psvdata_ready) == 1) {
		/** Есть данные для передачи */
		pthread_mutex_lock(&mx_psv);

		dst = (unsigned int)rxBuf.buf[0];

		prepare_cadr(&tx2Buf, rxBuf.buf, rxBuf.wpos);
			
		put_state(&psvdata_ready, 0);
		pthread_cond_signal(&psvdata);
		
		pthread_mutex_unlock(&mx_psv);
		
	} else {
		ans = bo_getFifoVal(buf, bufSize);
		if (ans > 0) {
			dst = (unsigned int)buf[0];

			prepare_cadr(&tx2Buf, (char *)buf, ans);

		} else {
			/*
			bo_log("send_dataToPassive()/bo_getFifoVal(): <= 0");
			*/
			return 0;
		}
	}
	
	while (nretr--) {
		
		/** Данные для передачи подготовлены */
		res = trx2(targ);
		if (res < 0) return -1;

		if ((get_rxFl(&rx2Buf) >= RX_DATA_READY) &&
		    ((rx2Buf.buf[1] & 0xFF) == dst)) {
			switch (get_rxFl(&rx2Buf)) {
			case RX_DATA_READY:
				/** Ответ пассивного устройства
				 * загрузить в лог. */
				putLog();
				return 0;
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("send_dataToPassive(): Cadr Error !");
				break;
			case RX_TIMEOUT:
				/** Текущее устройство не успело дать ответ. */
				bo_log("send_dataToPassive(): timeout dst= %d", dst);
				break;
			default:
				bo_log("send_dataToPassive(): state ??? fl= %d",
				       get_rxFl(&rx2Buf));
				break;
			}
		}
	}
	/** Текущее устройство не отвечает, вычеркиваем его из списка. */
	remf_rtbl(targ, &dst2Buf, dst);

	return 0;
}


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

void *rtbl_send(void *arg)
{
	struct rt_thread_arg *targ = (struct rt_thread_arg *)arg;
	
	while (1) {
		rtSend_sock = bo_setConnect(targ->ip, targ->port);
		
		if (rtSend_sock < 0) {
			bo_log("rtbl_send: bo_setConnect ERROR");
			sleep(10);
			continue;
		}
		
		bo_log("rtbl_send(): socket ok");
		
		while (1) {
			/* write (1, "rtbl_send: bo_setConnect
			   leaving\n", 33); */
			sleep(10);
		}
	}
	
	bo_log("rtbl_send: exit");
	pthread_exit(0);
}

void *chan2(void *arg)
{
	struct chan_thread_arg *targ = (struct chan_thread_arg *)arg;
	struct timeval t;
	int count_scan = targ->tscan;
	int dst;
	int res;
	/* 
	   char *defv = "null";
	   char *rt_key;
	   char *val;
	   int i; */
	
	while (1) {

		t.tv_sec = targ->tsec;
		t.tv_usec = targ->tusec;
		select(0, NULL, NULL, NULL, &t);
		
		if (count_scan == targ->tscan) {
			count_scan = 0;
			if (targ->ch2_enable) {
				/** Сканируем устройства порт 2 RS485
				 *  (1 раз в сек) */
				write (1, "chan2: 1000ms\n", 14);
				dst = targ->dst_beg;
				while (dst <= targ->dst_end) {
					res = scan2(targ, dst);
					if (res < 0) {
						bo_log("chan2: exit");
						pthread_exit(0);
					}
				
					dst++;
				}
				write(1, "\n", 1);
				
			} else {
				write (1, "chan2: disabled\n", 16);
			}
			
			/** Тест глобальной таблицы маршр.
			for (i=0; i<rt_getsize(rtg); i++) {
				rt_key = rt_getkey(rtg, i);
				if (rt_key == NULL) continue;
				val = ht_get(rtg, rt_key, defv);
				printf("rtg/%s:%s\n", rt_key, val);
			} */
			
			/** Тест локальной таблицы маршр.
			for (i=0; i<rt_getsize(rtl); i++) {
				rt_key = rt_getkey(rtl, i);
				if (rt_key == NULL) continue;
				val = ht_get(rtl, rt_key, defv);
				printf("rtl/%s:%s\n", rt_key, val);
			} */
		}
		
		if (targ->ch2_enable) {
			/** Запросы из FIFO пассивным устройствам на порту 2
			 *  (1 раз в 50 мсек) */
			if (dst2Buf.wpos > 0)
				res = send_dataToPassive(targ);
		
			if (res < 0) break;
		}
		
#ifdef MOXA_TARGET
			if (targ->wdt_en)
				put_wdtlife(&wdt_life, WDT_CHAN2);
#endif
		
		count_scan++;
	}
	
	bo_log("chan2: exit");
	pthread_exit(0);
}

