/*
 *		Moxa-slave канал 1 (активные устройства)
 *
 * Version:	@(#)ch1_threads.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */


#include "slave.h"
#include "ort.h"
#include "bologging.h"


/**
 * trx - Передача кадра данных активному устройству сети RS485 и
 *       получение от него ответа.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @return  0- успех, -1 неудача.
 */
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

/**
 * scan - Опрос устройств на сети RS485 (порт 1).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 * Опрос производится в пределах заданного диапазона адресов,
 * прописанного в конфигурационном файле.
 * Если устройство отвечает на запрос в пределах отведенного времени, то
 * его адрес прописывается в локальной таблице маршрутов.
 * Если устройство, не отвечает на запрос, то оно удаляется из
 * локальной таблицы маршрутов.
 */
int scan(struct chan_thread_arg *targ, int dst)
{
	int res;
#ifdef __PRINT__
	char a[4];
#endif

	prepare_cadr_scan(targ, &txBuf, dst);

	/** Данные для передачи подготовлены */
	res = trx(targ);
	if (res < 0) return -1;
	
	if ((get_rxFl(&rxBuf) >= RX_DATA_READY) &&
	    ((rxBuf.buf[1] & 0xFF) == dst)) {
		switch (get_rxFl(&rxBuf)) {
		case RX_DATA_READY:
			put_rtbl(targ, &dstBuf, dst);
#ifdef __PRINT__
			memset(a, 0, 3);		
			sprintf(a, "%d", dst);
			write(1, a, 4);
#endif
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
#ifdef __PRINT__
			write(1, ".", 1);
#endif
			break;
		default:
			bo_log("scan(): state ??? fl= %d", get_rxFl(&rxBuf));
			break;
		}
	} else {
#ifdef __PRINT__
		write(1, "-", 1);
#endif
	}

	return 0;
}

/**
 * active_netStat - Запрос конфигурации сети RS485.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 * Если устройство, не отвечает на запрос, то оно (после определенного
 * количества попыток) удаляется из локальной таблицы маршрутов.
 */
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


/**
 * active_getLog - Запрос лога.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 * Если устройство, не отвечает на запрос, то оно (после определенного
 * количества попыток) удаляется из локальной таблицы маршрутов.
 */
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

/**
 * active - Запросы активным устройствам на сети RS485 (порт 1).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 * Если устройство отвечает на запрос в пределах отведенного времени, то,
 * в зависимости от месторасположения адресата, имеем следующие
 * варианты действий, ответ адресован:
 *  1) локальному пассивному устройству -
 *     ответ перенаправляется на канал 2 (пассивные устройства);
 *  2) пассивному устройству чужого узла -
 *     ответ передается на стек FIFO соответствующего узла;
 *  3) собственному сетевому контроллеру -
 *     ответ обрабатывает сам контроллер.
 *
 * Если устройство, не отвечает на запрос, то оно (после определенного
 * количества попыток) удаляется из локальной таблицы маршрутов.
 */
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
						pthread_mutex_lock(&mx_psv);

						/** имеем что передать пассивному устройству -
						 * ch2_threads.c: send_dataToPassive() */
						put_state(&psvdata_ready, 1);
	
						/** Ожидаем готовности послать кадр
						    пассивному устройствуву */
						while (get_state(&psvdata_ready))
							pthread_cond_wait(&psvdata, &mx_psv);

						pthread_mutex_unlock(&mx_psv);
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


/**
 * chan1 - Канал 1 (активные устройства).
 * @arg:  Параметры для потока.
 *
 * Поток выполняет следующие действия:
 * 1) Организует таймеры 50ms и 1000ms;
 * 2) По таймеру 50ms производит запросы активным устройствам;
 * 3) По таймеру 1000ms опрашивает устройства на порту 1.
 */
void *chan1(void *arg)
{
	struct chan_thread_arg *targ = (struct chan_thread_arg *)arg;
	struct timeval t;
	int count_scan = targ->tscan;
	int dst;
	int res;
	
	while (1) {
		t.tv_sec = targ->tsec;
		t.tv_usec = targ->tusec;
		select(0, NULL, NULL, NULL, &t);
		
		if (count_scan == targ->tscan) {
			count_scan = 0;
			if (targ->ch1_enable) {
				/** Сканируем устройства порт 1 RS485
				 *  (1 раз в сек) */
#ifdef __PRINT__
				write (1, "chan1: 1000ms\n", 14);
#endif
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
#ifdef __PRINT__
				write(1, "\n", 1);
#endif
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
		
#if defined (__MOXA_TARGET__) && defined (__WDT__)
			if (targ->wdt_en)
				put_wdtlife(&wdt_life, WDT_CHAN1);
#endif
		count_scan++;
	}
	
	bo_log("chan1: exit");
	pthread_exit(0);
}

