/*
 *		Moxa-slave канал 2 (пассивные устройства)
 *
 * Version:	@(#)ch2_threads.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */


#include "slave.h"
#include "ort.h"
#include "bo_fifo.h"
#include "bologging.h"


/**
 * passive_TxRx - Цикл обмена данными по сети RS485.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @msg:
 * @return  0- успех, -1 неудача, 1- устройство не ответило.
 *
 * Если устройство, не отвечает на запрос, то оно (после определенного
 * количества попыток) удаляется из локальной таблицы маршрутов.
 */
int passive_TxRx(struct chan_thread_arg *targ, int dst, char *msg)
{
	int res;
	int rxer = 0;
	int i;
	int n = 1;    /** При использовании n<<i в цикле for имеем
		       * ряд 1,2,4.. для последующего увеличения
		       * времени таймаута при приеме данных по каналу RS485 */
	
	for (i=0; i<targ->nretries; i++) {
		
		/** Передача */
		res = tx(targ, &tx2Buf, msg);
		if (res < 0) return -1;
		
		/** Прием */
		res = rx(targ, &rx2Buf, targ->tout*(n<<i), msg);
		if (res < 0) return -1;
		
		if (get_rxFl(&rx2Buf) >= RX_DATA_READY) {
			switch (get_rxFl(&rx2Buf)) {
			case RX_DATA_READY:
				/** Ответ пассивного устройства */
				return 0;
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("passive_TxRx(%s): Cadr Error !", msg);
				rxer = 1;
				break;
			case RX_TIMEOUT:
				/** Текущее устройство
				 * не успело дать ответ. */
				bo_log("passive_TxRx(%s): timeout dst= %d",
				       msg, dst);
				rxer = 1;
				break;
			default:
				bo_log("passive_TxRx(%s): state ??? fl= %d",
				       msg, get_rxFl(&rx2Buf));
				rxer = 1;
				break;
			}
		}
	}
	
	bo_log("passive_TxRx(%s): get_rxFl(&rx2Buf)=[%d], rx2Buf.buf[1]=[%d], dst=[%d]",
	       msg, get_rxFl(&rx2Buf), rx2Buf.buf[1], dst);
	
	if (rxer)
		/** Текущее устройство вычеркиваем из списка. */
		remf_rtbl(targ, &dst2Buf, dst);
	else if (get_rxFl(&rx2Buf) == RX_DATA_READY) {
		return -1;
	}
	
	return 1;
}


/**
 * dataToPassive - Запросы пассивным устройствам на сети RS485 (порт 2).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @return  0- успех, -1 неудача.
 *
 * В зависимости от источника поступления запроса, могут возникнуть 2 ситуации:
 *  1) Переменная psv_local_stage == PSVL_BEGIN -
 *     поступил запрос от локального активного устройства (канал 1);
 *  2) Переменная psv_fifo_stage == PSVF_PROCESS -
 *     поступил запрос от стороннего узла на стеке FIFO;
 *
 */
int dataToPassive(struct chan_thread_arg *targ)
{
	unsigned int dst;
	int res = 0;
	int stage;
	char key[4];  /** Адрес активного у-ва */
	
	if (get_state(&psv_local_stage) == PSVL_BEGIN) {
		/** Есть данные для передачи пассивному устройству */
		pthread_mutex_lock(&mx_psv);

		dst = (unsigned int)rxBuf.buf[0];

		prepare_cadr(&tx2Buf, rxBuf.buf, rxBuf.wpos);
		
		put_state(&psv_local_stage, PSVL_PROCESS);
		pthread_cond_signal(&psvdata);
		
		pthread_mutex_unlock(&mx_psv);
		
		/** Данные для передачи подготовлены */
		res = passive_TxRx(targ, dst, "2pasFromAct");
		if (res < 0) return -1;
		
		if (res == 0) {
			/** Кадр сети RS485 (local node) */
			stage = PSVL_OK;
			res = 0;
		} else if (res == 1) {
			/** Если пассивное устройство не ответило */
			stage = PSVL_ERROR;
			res = 0;
		}
		
		pthread_mutex_lock(&mx_psv);

		put_state(&psv_local_stage, stage);
		
		/** Ожидаем готовности послать кадр ответа
		    активному устройствуву */
		while (get_state(&psv_local_stage) == stage)
			pthread_cond_wait(&psvAnsdata, &mx_psv);
			
		pthread_mutex_unlock(&mx_psv);
	}
	
	if (get_state(&psv_fifo_stage) == PSVF_PROCESS) {
		/** В FIFO есть данные для передачи пассивному устройству */
		dst = (unsigned int)getFifo_buf[0];
		
		prepare_cadr(&tx2Buf, (char *)getFifo_buf, getFifo_ans);
		
		/** Данные для передачи подготовлены */
		res = passive_TxRx(targ, dst, "2fifo");
		if (res < 0) return -1;
		
		if (res == 0) {
			/** Кадр сети RS485 (FIFO) */
			memset(key, 0, 3);
			sprintf(key, "%03d", (unsigned char)rx2Buf.buf[0]);
			prepareFIFO(&rx2Buf, key, (unsigned char)rx2Buf.buf[0]);
			res = 0;
		} else if (res == 1) {
			/** Если пассивное устройство не ответило */
			res = 0;
		}
		
		put_state(&psv_fifo_stage, PSVF_FREE);
	}
	
	return res;
}


/**
 * chan2 - Канал 2 (пассивные устройства).
 * @arg:  Параметры для потока.
 *
 * Поток выполняет следующие действия:
 * 1) Производит запросы пассивным устройствам;
 * 2) Опрашивает устройства на порту 2.
 */
void *chan2(void *arg)
{
	struct chan_thread_arg *targ = (struct chan_thread_arg *)arg;
	int count_scan = targ->tscan;
	int fscan = 1;
	int scan_dst = targ->dst_beg;
	int res;

	while (1) {

		if (targ->enable) {
			if (count_scan == targ->tscan) {
				count_scan = 0;
				/** Сканируем устройства порт 2 RS485
				 *  (1 раз в сек) */
#ifdef __PRINT__
				write (1, "chan2: 1000ms\n", 14);
#endif
				if (fscan) {
					fscan = 0;
					while (scan_dst <= targ->dst_end) {
						res = scan(targ,
							   &tx2Buf,
							   &rx2Buf,
							   &dst2Buf,
							   scan_dst,
							   "2scan");
						
						if (res < 0) {
							bo_log("chan2: exit");
							pthread_exit(0);
						}
						
						scan_dst++;
					}
					scan_dst = targ->dst_beg;
				} else {
					res = scan(targ,
						   &tx2Buf,
						   &rx2Buf,
						   &dst2Buf,
						   scan_dst,
						   "2scan");
					
					if (res < 0) {
						bo_log("chan2: exit");
						pthread_exit(0);
					}
					
					if (scan_dst < targ->dst_end)
						scan_dst++;
					else
						scan_dst = targ->dst_beg;
				}
#ifdef __PRINT__
				write(1, "\n", 1);
#endif				
			}
			/** Запросы из FIFO пассивным устройствам на порту 2
			 *  (1 раз в 50 мсек) */

			if (dst2Buf.wpos > 0) {
				/** Если пассивные устройства имеются
				 * на шине, то разрешаем проверить
				 * наличие запросов к ним. */
				res = dataToPassive(targ);
				if (res < 0) break;
				
			} else {
				pthread_mutex_lock(&mx_psv);
				
				if (get_state(&psv_local_stage) != PSVL_FREE) {
					put_state(&psv_local_stage, PSVL_FREE);
					pthread_cond_signal(&psvdata);
				}
				
				pthread_mutex_unlock(&mx_psv);
				
				usleep(10000);
			}
		} else
			usleep(10000);
		
		if (targ->usleep != 0)
			usleep(targ->usleep);
		
#if defined (__MOXA_TARGET__) && defined (__WDT__)
		if (targ->wdt_en)
			put_wdtlife(&wdt_life, WDT_CHAN2);
#endif
		count_scan++;
	}
	
	bo_log("chan2: exit");
	pthread_exit(0);
}

