/*
 *		Moxa-slave канал 2 (пассивные устройства)
 *
 * Version:	@(#)ch2_threads.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */


#include "slave.h"
#include "ort.h"
#include "bologging.h"
#include "bo_fifo.h"


/**
 * passive_process - Обработка ответов от пассивных устройств на сети RS485.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 * Устройство отвечает на запрос в пределах отведенного времени.
 * В зависимости от месторасположения адресата, имеем следующие
 * варианты действий, ответ адресован:
 *  1) локальному пассивному устройству -
 *     ответ перенаправляется на канал 2 (пассивные устройства);
 *  2) пассивному устройству чужого узла -
 *     ответ передается на стек FIFO соответствующего узла;
 *  3) собственному сетевому контроллеру -
 *     ответ обрабатывает сам контроллер active_getLog(), active_netStat().
 */
int passive_process(struct chan_thread_arg *targ, int dst)
{
	char key[4];  /** Адрес активного у-ва */
	int res = 0;
	
	memset(key, 0, 3);
	/** Адрес активного у-ва */
	sprintf(key, "%03d", (unsigned char)rx2Buf.buf[0]);
	
	if (test_bufDst(&dstBuf, (unsigned char)rx2Buf.buf[0]) != -1) {
		if (targ->ch1_enable) {
			/** Кадр сети RS485 (local node) */
			pthread_mutex_lock(&mx_psv);

			prepare_cadr(&txBuf, rx2Buf.buf, rx2Buf.wpos);
			
			/** имеем ответ активному устройству -
			 * ch1_threads.c: send_dataToPassive() */
			put_state(&psvAnsdata_ready, 0);
			pthread_cond_signal(&psvAnsdata);
			
			pthread_mutex_unlock(&mx_psv);
			
		} else
			bo_log("passive(): key= [%s] ch1 disable", key);
		
	} else if (rt_iskey(rtg, key)) {
		/** Кадр сети RS485 (FIFO) */		
		prepareFIFO(&rx2Buf, key, (unsigned char)rx2Buf.buf[0]);
	} else {
		bo_log("passive(): key= [%s] ???", key);
		/** Если пассивное устройство не ответило */
		pthread_mutex_lock(&mx_psv);
		
		put_state(&psvAnsdata_ready, 3);
		pthread_cond_signal(&psvAnsdata);
		
		pthread_mutex_unlock(&mx_psv);
	}
	
	return res;
}

/**
 * passiveFromActive - Запросы пассивным устройствам на сети RS485 (порт 2).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @return  0- успех, -1 неудача.
 *
 * В зависимости от источника поступления запроса, могут возникнуть 2 ситуации:
 *  1) Переменная psvdata_ready == 1 -
 *     поступил запрос от локального активного устройства (канал 1);
 *  2) Переменная psvdata_ready == 0 -
 *     проверяем наличие запросов от стороннего узла на стеке FIFO;
 * 
 * Если запрос поступил, то формируем кадр и выполняем запрос.
 * После успешного прохождения запроса и получения ответа, помещаем
 * ответ в логе на сервере мастера.
 *
 * Если устройство, не отвечает на запрос, то оно (после определенного
 * количества попыток) удаляется из локальной таблицы маршрутов.
 */
int passiveFromActive(struct chan_thread_arg *targ)
{
	unsigned int dst;
	int res;
	int i;
	int n = 1;    /** При использовании n<<i в цикле for имеем
		       * ряд 1,2,4.. для последующего увеличения
		       * времени таймаута при приеме данных по каналу RS485 */
		
	if (get_state(&psvdata_ready) == 0)
		return 0;
	
	/** Есть данные для передачи пассивному устройству */
	pthread_mutex_lock(&mx_psv);

	dst = (unsigned int)rxBuf.buf[0];

	prepare_cadr(&tx2Buf, rxBuf.buf, rxBuf.wpos);
	
	/** Запрос для пассивного устройства загрузить в лог. */
	putLog();
	
	put_state(&psvdata_ready, 0);
	pthread_cond_signal(&psvdata);
	
	pthread_mutex_unlock(&mx_psv);
	
	
	/** Данные для передачи подготовлены */
	
	for (i=0; i<targ->nretries; i++) {
		/** Передача */
		res = tx(targ, &tx2Buf, "2pasFact");
		if (res < 0) return -1;
		
		/** Прием */
		res = rx(targ, &rx2Buf, targ->tout*(n<<i), "2pasFact");
		if (res < 0) return -1;
		
		if ((get_rxFl(&rx2Buf) >= RX_DATA_READY) &&
		    ((rx2Buf.buf[1] & 0xFF) == dst)) {
			switch (get_rxFl(&rx2Buf)) {
			case RX_DATA_READY:
				/** Ответ пассивного устройства */
				return passive_process(targ, dst);
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("send_dataToPassive(): Cadr Error !");
				break;
			case RX_TIMEOUT:
				/** Текущее устройство не успело дать ответ. */
				bo_log("send_dataToPassive(): timeout dst= %d",
				       dst);
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

	/** Если пассивное устройство не ответило */
	pthread_mutex_lock(&mx_psv);
	
	put_state(&psvAnsdata_ready, 3);
	pthread_cond_signal(&psvAnsdata);
	
	pthread_mutex_unlock(&mx_psv);
	
	return 0;
}

int data_FIFO(struct chan_thread_arg *targ)
{
	int dst;
	int res;
	int i;
	int n = 1;    /** При использовании n<<i в цикле for имеем
		       * ряд 1,2,4.. для последующего увеличения
		       * времени таймаута при приеме данных по каналу RS485 */

	/**
	   bo_log("data_FIFO  bo_getFifoVal(): before");
	*/
	getFifo_ans = bo_getFifoVal(getFifo_buf, BO_FIFO_ITEM_VAL);
	if (getFifo_ans <= 0) {
		/** Если в FIFO нет данных
		bo_log("data_FIFO bo_getFifoVal(): no data");
		usleep(50000); */
		printf("bo_getFifoVal(): NO DATA / getFifo_ans= [%d]\n", getFifo_ans);
		
		return 0;
	}
	/**
	   bo_log("data_FIFO bo_getFifoVal(): after");
	*/
	/** В FIFO есть данные для передачи пассивному устройству */
	dst = (unsigned int)getFifo_buf[0];

	if (test_bufDst(&dstBuf, dst) != -1) {
		if (targ->ch1_enable) {
			/** Кадр сети RS485 (alien node) */
			pthread_mutex_lock(&mx_actFIFO);

			/** имеем ответ активному устройству -
			 * ch1_threads.c: active() */
			put_state(&actFIFOdata_ready, 1);
			
			/** Ожидаем готовности послать кадр
			    активному устройствуву */
			while (get_state(&actFIFOdata_ready))
				pthread_cond_wait(&actFIFOdata, &mx_actFIFO);

			pthread_mutex_unlock(&mx_actFIFO);
		} else
			bo_log("data_FIFO(): ch1 disable");
		
		return 0;
	}
	
	prepare_cadr(&tx2Buf, (char *)getFifo_buf, getFifo_ans);

	/** Запрос для пассивного устройства загрузить в лог.
	bo_log("data_FIFO after putLog()"); */
	putLog();

	/**
	bo_log("data_FIFO before TX [%d %d %d %d %d %d %d %d %d %d]",
	       tx2Buf.buf[0],
	       tx2Buf.buf[1],
	       tx2Buf.buf[2],
	       tx2Buf.buf[3],
	       tx2Buf.buf[4],
	       tx2Buf.buf[5],
	       tx2Buf.buf[6],
	       tx2Buf.buf[7],
	       tx2Buf.buf[8],
	       tx2Buf.buf[9]
		);
	*/
	/** Данные для передачи подготовлены */
	
	for (i=0; i<targ->nretries; i++) {
		/** Передача */
		res = tx(targ, &tx2Buf, "2fifo");
		if (res < 0) return -1;
	
		/** Прием */
		res = rx(targ, &rx2Buf, targ->tout*(n<<i), "2fifo");
		if (res < 0) return -1;
		
		if ((get_rxFl(&rx2Buf) >= RX_DATA_READY) &&
		    ((rx2Buf.buf[1] & 0xFF) == dst)) {
			switch (get_rxFl(&rx2Buf)) {
			case RX_DATA_READY:
				/** Ответ пассивного устройства
				bo_log("data_FIFO after RX [%d %d %d %d %d %d %d %d %d %d]",
				       rx2Buf.buf[0],
				       rx2Buf.buf[1],
				       rx2Buf.buf[2],
				       rx2Buf.buf[3],
				       rx2Buf.buf[4],
				       rx2Buf.buf[5],
				       rx2Buf.buf[6],
				       rx2Buf.buf[7],
				       rx2Buf.buf[8],
				       rx2Buf.buf[9]
					); */
				
				return passive_process(targ, dst);
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("data_FIFO(): Cadr Error !");
				break;
			case RX_TIMEOUT:
				/** Текущее устройство не успело дать ответ. */
				bo_log("data_FIFO(): timeout dst= %d", dst);
				break;
			default:
				bo_log("data_FIFO(): state ??? fl= %d",
				       get_rxFl(&rx2Buf));
				break;
			}
		}
	}
	/** Текущее устройство не отвечает, вычеркиваем его из списка. */
	remf_rtbl(targ, &dst2Buf, dst);

	return 0;
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
	int dst;
	int res;

	while (1) {
		/* usleep(50000); */

		if (count_scan == targ->tscan) {
			count_scan = 0;
			if (targ->ch2_enable) {
				/** Сканируем устройства порт 2 RS485
				 *  (1 раз в сек) */
#ifdef __PRINT__
				write (1, "chan2: 1000ms\n", 14);
#endif
				/**
				bo_log("ch2(): scan");
				*/
				dst = targ->dst_beg;
				while (dst <= targ->dst_end) {
					res = scan(targ,
						   &tx2Buf,
						   &rx2Buf,
						   &dst2Buf,
						   dst,
						   "2scan");
					if (res < 0) {
						bo_log("chan2: exit");
						pthread_exit(0);
					}
					
					dst++;
				}
#ifdef __PRINT__
				write(1, "\n", 1);
#endif				
			}
		}
		
		if (targ->ch2_enable) {
			/** Запросы из FIFO пассивным устройствам на порту 2
			 *  (1 раз в 50 мсек) */

			if (dst2Buf.wpos > 0) {
				/** Если пассивные устройства имеются
				 * на шине, то разрешаем проверить
				 * наличие запросов к ним. */
				res = passiveFromActive(targ);
				if (res < 0) break;
			}
		}
		
		res = data_FIFO(targ);
		if (res < 0) break;
		
#if defined (__MOXA_TARGET__) && defined (__WDT__)
			if (targ->wdt_en)
				put_wdtlife(&wdt_life, WDT_CHAN2);
#endif
		
		count_scan++;
	}
	
	bo_log("chan2: exit");
	pthread_exit(0);
}

