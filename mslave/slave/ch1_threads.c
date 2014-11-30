
#include "ch_threads.h"
#include "slave.h"
#include "ort.h"
#include "bologging.h"
#include "bo_net.h"
#include "bo_fifo.h"
#include "bo_net_fifo_server.c"


int trx(struct chan_thread_arg *targ)
{
	char tbuf[BUF485_SZ];  /** Буфер передатчика RS485 */
	char buf[BUF485_SZ];  /** Буфер приемника RS485 */
	int res;

	/** Tx */
	res = writer(&txBuf, tbuf, targ->port);	
	if (res < 0) return -1;

	/** Rx */	
	put_rxFl(&rxBuf, RX_WAIT);  /** Состояние приема - 'ожидание данных' */
	rxBuf.wpos = 0;  /** Позиция записи в начало буфера приемника */
	res = 1;
	
	while (res) {
		res = reader(&rxBuf, buf, targ->port, targ->tout);
		if (res < 0) return -1;
	}
	
	return 0;
}

int scan(struct chan_thread_arg *targ, int dst)
{
	char a[4];
	int res;

	prepare_cadr_scan(targ, &txBuf, dst);

	/** Данные для передачи подготовлены */
	res = trx(targ);
	if (res < 0) return -1;
	
	if ((get_rxFl(&rxBuf) >= RX_DATA_READY) &&
	    ((rxBuf.buf[1] & 0xFF) == dst)) {
		switch (get_rxFl(&rxBuf)) {
		case RX_DATA_READY:
			put_rtbl(targ, &dstBuf, dst);
			
			memset(a, 0, 3);		
			sprintf(a, "%d", dst);
			write(1, a, 4);
			
			break;
		case RX_ERROR:
			/** Ошибка кадра */
			bo_log("scan(): Cadr Error !");
			break;
		case RX_TIMEOUT:
			/** Текущее устройство не отвечает,
			 * вычеркиваем его из списка. */
			bo_log("scan(): timeout dst= %d", dst);
			remf_rtbl(targ, &dstBuf, dst);

			write(1, ".", 1);
			break;
		default:
			bo_log("scan(): state ??? fl= %d", get_rxFl(&rxBuf));
			break;
		}
	} else {
		write(1, "-", 1);
	}

	return 0;
}

int active_netStat(struct chan_thread_arg *targ, int dst)
{
	int res;
	int nretr = targ->nretries;

	prepare_cadr_actNetStat(targ, &txBuf, dst);
	
	while (nretr--) {
		
		/** Данные для передачи подготовлены */
		res = trx(targ);
		if (res < 0) return -1;
		
		if ((get_rxFl(&rxBuf) >= RX_DATA_READY) &&
		    ((rxBuf.buf[1] & 0xFF) == dst)) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				/* bo_log("active_netStat(): ok"); */
				return 0;
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("active_netStat(): Cadr Error !");
				break;
			case RX_TIMEOUT:
				/** Текущее устройство не успело дать ответ. */
				bo_log("active_netStat(): timeout dst= %d", dst);
				break;
			default:
				bo_log("active_netStat(): state ??? fl= %d",
				       get_rxFl(&rxBuf));
				break;
			}
		}
	}
	/** Текущее устройство не отвечает, вычеркиваем его из списка. */
	remf_rtbl(targ, &dstBuf, dst);
	
	return 0;
}

void active_toPassive()
{
	pthread_mutex_lock(&mx_psv);

	/** имеем что передать пассивному устройству -
	 * ch2_threads.c: send_dataToPassive() */
	put_state(&psvdata_ready, 1);
	
	/** Ожидаем готовности послать кадр
	    пассивному устройствуву */
	while (get_state(&psvdata_ready))
		pthread_cond_wait(&psvdata, &mx_psv);

	pthread_mutex_unlock(&mx_psv);
}

int active_getLog(struct chan_thread_arg *targ, int dst)
{
	int res;
	int nretr = targ->nretries;

	prepare_cadr_quLog(targ, &txBuf, dst);
	
	while (nretr--) {
		
		/** Данные для передачи подготовлены */
		res = trx(targ);
		if (res < 0) return -1;
		
		if ((get_rxFl(&rxBuf) >= RX_DATA_READY) &&
		    ((rxBuf.buf[1] & 0xFF) == dst)) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				/* bo_log("active_getLog(): ok"); */
				return 0;
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("active_getLog(): Cadr Error !");
				break;
			case RX_TIMEOUT:
				/** Текущее устройство не успело дать ответ. */
				bo_log("active_getLog(): timeout dst= %d", dst);
				break;
			default:
				bo_log("active_getLog(): state ??? fl= %d",
				       get_rxFl(&rxBuf));
				break;
			}
		}
	}
	/** Текущее устройство не отвечает, вычеркиваем его из списка. */
	remf_rtbl(targ, &dstBuf, dst);
	
	return 0;
}

int active(struct chan_thread_arg *targ, int dst)
{
	char key[4];  /** Адрес пассивного у-ва */
	int res;
	int nretr = targ->nretries;
	
	prepare_cadr_act(targ, &txBuf, dst);

	while (nretr--) {

		/** Данные для передачи подготовлены */
		res = trx(targ);
		if (res < 0) return -1;
		
		if ((get_rxFl(&rxBuf) >= RX_DATA_READY) &&
		    ((rxBuf.buf[1] & 0xFF) == dst)) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				
				memset(key, 0, 3);
				/** Адрес пассивного у-ва */
				sprintf(key, "%03d", rxBuf.buf[0]);

				if (rxBuf.buf[0] == targ->src) {
					/** Ответ активного устройства
					 * для сетевого контроллера */
					if (rxBuf.buf[2] == targ->cdquLogId) {
						/** GetLog */
						return active_getLog(targ, dst);
					} else if (rxBuf.buf[2] == targ->cdnsId) {
						/** Запрос о составе сети RS485 */
						return active_netStat(targ, dst);
					} else if (rxBuf.buf[4] == 0) {
						/** OPERATION_COMPLETED */
						bo_log("active(): OPERATION_COMPLETED ");
					} else if (rxBuf.buf[4] == 7) {
						/** WRONG_REQUEST */
						bo_log("active(): WRONG_REQUEST ");
					} else {
						/** chertechto */
						bo_log("active(): chertechto ");
					}
					
				} else if (test_bufDst(&dst2Buf, rxBuf.buf[0])) {
					if (targ->ch2_enable) {
						/** Кадр сети RS485 (local node) */
						active_toPassive();
					} else
						bo_log("active(): key= [%s] ch2 disable", key);
					
				} else if (rt_iskey(rtg, key)) {
					/** Кадр сети RS485 (FIFO) */
					sendFIFO(targ->fifo_port, key);
				} else {
					bo_log("active(): key= [%s] ???", key);
				}
				
				return 0;
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("active(): Cadr Error !");
				break;
			case RX_TIMEOUT:
				/** Текущее устройство не успело дать ответ. */
				bo_log("active(): timeout dst= %d", dst);
				break;
			default:
				bo_log("active(): state ??? fl= %d", get_rxFl(&rxBuf));
				break;
			}
		}
	}
	/** Текущее устройство не отвечает, вычеркиваем его из списка. */
	remf_rtbl(targ, &dstBuf, dst);
	
	return 0;
}

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

void *chan1(void *arg)
{
	struct chan_thread_arg *targ = (struct chan_thread_arg *)arg;
	struct timeval t;
	int count_scan = targ->tscan;
	int dst;
	int res;

	while (1) {
		logSend_sock = bo_setConnect(targ->logSend_ip, targ->logSend_port);
		if (logSend_sock < 0) {
			bo_log("chan1(): logSend_sock=bo_setConnect() ERROR");
			sleep(10);
			continue;
		}
		break;
	}
	
	bo_log("logSend_sock: socket ok");
	
	while (1) {
		t.tv_sec = targ->tsec;
		t.tv_usec = targ->tusec;
		select(0, NULL, NULL, NULL, &t);
		
		if (count_scan == targ->tscan) {
			count_scan = 0;
			if (targ->ch1_enable) {
				/** Сканируем устройства порт 1 RS485
				 *  (1 раз в сек) */
				write (1, "chan1: 1000ms\n", 14);
				dst = targ->dst_beg;
				while (dst <= targ->dst_end) {
					res = scan(targ, dst);
					if (res < 0) {
						bo_log("chan1: exit");
						pthread_exit(0);
					}
					
					dst++;
				}

				dstBuf.rpos = 0;

				write(1, "\n", 1);
				
			} else {
				write (1, "chan1: disabled\n", 16);
			}
		}

		if (targ->ch1_enable) {
			dst = get_bufDst(&dstBuf);
			if (dstBuf.wpos > 0) {
				/** Передача конфигурации пассивных
				 * устройств */
				res = active_netStat(targ, dst);
				if (res < 0) break;
				
				/** Разрешение активным устройствам на
				 *  порту 1 (1 раз в 50 мсек) */
				res = active(targ, dst);		
				if (res < 0) break;
			}
		}
		
#ifdef MOXA_TARGET
			if (targ->wdt_en)
				put_wdtlife(&wdt_life, WDT_CHAN1);
#endif
		
		count_scan++;
	}
	
	bo_log("chan1: exit");
	pthread_exit(0);
}

#ifdef MOXA_TARGET

void *wdt(void *arg)
{
	struct wdt_thread_arg *targ = (struct wdt_thread_arg *)arg;
	struct timeval t;
	char a[4];
	
	while (1) {
		t.tv_sec = targ->tsec;
		t.tv_usec = targ->tusec;
		select(0, NULL, NULL, NULL, &t);
		
		if (targ->wdt_en) {
			memset(a, 0, 4);		
			sprintf(a, "%d", get_wdtlife(&wdt_life));
			write (1, "\n", 1);
			write(1, a, 4);
			write (1, "\n", 1);
			if (get_wdtlife(&wdt_life) == WDT_LIVING) {
				/** Если контрольные участки кода пройдены,
				 * подтвердить работоспособность системы */
				set_wdtlife(&wdt_life, 0);
				/* swtd_ack(wdt_fd); */
				/* refresh the timer */
				write (1, "WDT refresh -----------------\n", 30);
				mxwdg_refresh(wdt_fd);
			} else
				write (1, "------------- WDT not refresh\n", 30);
		}
	}
	
	bo_log("wdt: exit");
	pthread_exit(0);
}

#endif

