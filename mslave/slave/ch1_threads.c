/*
 *		Moxa-slave канал 1 (активные устройства)
 *
 * Version:	@(#)ch1_threads.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */


#include "slave.h"
#include "ocrc.h"
#include "ort.h"
#include "bo_net_master_core_log.h"
#include "bo_cycle_arr.h"
#include "bo_parse.h"
#ifdef __SNMP__
#include "bo_snmp_mng.h"
#endif
#include "bologging.h"


/**
 * prepare_cadr_act - Формирование кадра для передающего буфера
 *                     (Разрешение активным устройствам на сети RS485).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @dst:  Адрес устройства.
 */
void prepare_cadr_act(struct chan_thread_arg *targ,
		      struct thr_tx_buf *b,
		      int dst)
{
	unsigned int crc;

	b->wpos = 0;
	put_txBuf(b, (char)dst);
	put_txBuf(b, (char)targ->src);

	put_txBuf(b, (char)targ->cdaId);
	put_txBuf(b, (char)targ->cdDest);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)0);

	crc = crc16modbus(b->buf, 6);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

/**
 * prepare_cadr_actNetStat - Формирование кадра для передающего буфера
 *                           (конфигурация устройств на сети RS485).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @dst:  Адрес устройства.
 *
 *       abit[32]:     0         1            31
 *
 *         bitAdr: 1000 0000 1000 0000 .. 0000 0000
 *                 |       | |       |    |       |
 *            adr: 8       1 15      9    256     249
 */
void prepare_cadr_actNetStat(struct chan_thread_arg *targ,
			     struct thr_tx_buf *b,
			     int dst)
{
	unsigned int crc;
	int i;
	unsigned int n = 0;
	char *key;
	char ip[16] = {0};
	int uadr;
	unsigned char abit[32] = {0};

	b->wpos = 0;
	put_txBuf(b, (char)dst);
	put_txBuf(b, (char)targ->src);

	put_txBuf(b, (char)targ->cdnsId);
	put_txBuf(b, (char)targ->cdDest);
	put_txBuf(b, (char)0);

	pthread_mutex_lock(&mx_rtg);
	for (i=0; i<rt_getsize(rtg); i++) {
		key = rt_getkey(rtg, i);
		if (key == NULL) continue;
		
		rt_getip(rtg, key, ip);
		if (ip[0] == 'N') continue;
		
		uadr = atoi(key);

		/** Получаем список адресов
		 * устройств на глобальном пространстве. */
		abit[(uadr-1)/8] |= (unsigned char)(1<<((uadr-1)%8));
		n++;
	}
	pthread_mutex_unlock(&mx_rtg);

	if (n == 0) {
		/** Если глобальная таблица маршрутов пуста, то
		 * используем данные локальной сети. */
		pthread_mutex_lock(&mx_rtl);
		for (i=0; i<rt_getsize(rtl); i++) {
			key = rt_getkey(rtl, i);
			if (key == NULL) continue;
			rt_getip(rtl, key, ip);
			if (ip[0] == 'N') continue;
			
			uadr = atoi(key);
			abit[(uadr-1)/8] |= (unsigned char)(1<<((uadr-1)%8));
			n++;
		}
		pthread_mutex_unlock(&mx_rtl);
	}

	n = 32;
	put_txBuf(b, (char)n);
	
	for (i=0; i<n; i++)
		put_txBuf(b, (char)abit[i]);

	crc = crc16modbus(b->buf, n+6);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

#ifdef __LOG__
/**
 * prepare_cadr_quLog - Формирование кадра для передающего буфера
 *                     (запрос лога).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @dst:  Адрес устройства.
 */
void prepare_cadr_quLog(struct chan_thread_arg *targ,
			struct thr_tx_buf *b,
			int dst)
{
	unsigned int crc;
	unsigned short nLogLine, qLogLine;
	int i, j;
	unsigned int nn;
	unsigned int logSz = 0;
	char buf[BO_ARR_ITEM_VAL];
	int bufSize = BO_ARR_ITEM_VAL;
	
	if (logSend_sock > 0) {

		b->wpos = 0;
		put_txBuf(b, (char)dst);
		put_txBuf(b, (char)targ->src);

		put_txBuf(b, (char)targ->cdquLogId);
		put_txBuf(b, (char)targ->cdDest);
		put_txBuf(b, (char)0);

		put_txBuf(b, (char)4);

		put_txBuf(b, rxBuf.buf[6]);
		put_txBuf(b, rxBuf.buf[7]);
		put_txBuf(b, rxBuf.buf[8]);
		put_txBuf(b, rxBuf.buf[9]);
		
		/** Ограничиваем запрашиваемый номер строки лога */
		nLogLine = ((rxBuf.buf[6] & 0xff) |
			    (rxBuf.buf[7] << 8)) % (targ->logMaxLines);
		qLogLine = (rxBuf.buf[8] & 0xff) | (rxBuf.buf[9] << 8);
		
		pthread_mutex_lock(&mx_sendSocket);
		
		for (j=0; j<qLogLine; j++) {
			nn = bo_master_core_logRecv(logSend_sock,
						    nLogLine+j,
						    buf,
						    bufSize);
			if (nn == -1) {
				bo_log("prepare_cadr_quLog(): %s",
					"bo_master_core_logRecv ERROR");
				break;
			} else if (nn > 0) {
				logSz += nn;
				for (i=0; i<nn; i++) {
					put_txBuf(b, (char)buf[i]);
				}
			} else {
				/**
				bo_log("prepare_cadr_quLog(): = 0");
				*/
			}
		}
		
		pthread_mutex_unlock(&mx_sendSocket);

		if (logSz > 0)
			/** Установить длину сообщения */
			set_txBuf(b, 5, (char)(logSz+4));
		
		crc = crc16modbus(b->buf, logSz+10);

		put_txBuf(b, (char)(crc & 0xff));
		put_txBuf(b, (char)((crc >> 8) & 0xff));
	}
}
#endif


#ifdef __SNMP__
/**
 * prepare_cadr_snmpStat - Формирование кадра для передающего буфера
 *                         (запрос состояния магистрали).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @dst:  Адрес устройства.
 */
void prepare_cadr_snmpStat(struct chan_thread_arg *targ,
			   struct thr_tx_buf *b,
			   int dst)
{
	unsigned int crc;
	char data[1024];
	char dt[32] = {0};
	char ip[16] = {0};
	
	struct OPT_SWITCH *o_sw;
	struct OPT_SWITCH *sw;
	struct PortItem *port;
	struct PortItem *prt;
	unsigned int snmpSz;
	int i, j, m;
	
	b->wpos = 0;
	put_txBuf(b, (char)dst);
	put_txBuf(b, (char)targ->src);

	put_txBuf(b, (char)targ->cdmsId);
	put_txBuf(b, (char)targ->cdDest);
	put_txBuf(b, (char)0);

	put_txBuf(b, (char)4);

	/** IP адрес */
	put_txBuf(b, rxBuf.buf[6]);
	put_txBuf(b, rxBuf.buf[7]);
	put_txBuf(b, rxBuf.buf[8]);
	put_txBuf(b, rxBuf.buf[9]);

	/** Длина сообщения */
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)0);

	memset(data, 0, 1024);

	
	snmpSz = 0;
	sprintf(ip, "%d.%d.%d.%d",
		rxBuf.buf[6],
		rxBuf.buf[7],
		rxBuf.buf[8],
		rxBuf.buf[9]);

	
	bo_snmp_lock_mut();
	
	o_sw = bo_snmp_get_tab();

	if (o_sw == NULL) {
		/** Нет данных */
	} else {
		/** "ip[%s] flg[%d] link[%d] speed[%d] descr[%s]\n" */
		for (i=0; i<targ->snmp_n; i++) {
			sw = o_sw + i;
			
			if (strcmp(sw->ip, ip)) continue;
			
			sprintf(dt, "\nip=[%15s]\n", sw->ip);
			strncat(data, dt, 22);

			snmpSz += 22;
			
			port = sw->ports;
			for (j=0; j<BO_OPT_SW_PORT_N; j++) {
				prt = port + j;
				
				sprintf(dt, "flg=[%03d] ", prt->flg);
				strncat(data, dt, 10);
				sprintf(dt, "link=[%01d] ", prt->link);
				strncat(data, dt, 9);
				sprintf(dt, "speed=[%03d] ", prt->speed);
				strncat(data, dt, 12);
				sprintf(dt, "descr=[%21s]\n", prt->descr);
				strncat(data, dt, 30);

				snmpSz += 10+9+12+30;
			}
		}
	}
	
	if (snmpSz > 0) {
		for (m=0; m<snmpSz; m++)
			put_txBuf(b, (char)data[m]);
		
		/** Установить длину сообщения */
		set_txBuf(b, 10, (char)(snmpSz & 0xff));
		set_txBuf(b, 11, (char)((snmpSz >> 8) & 0xff));
	}
	
	crc = crc16modbus(b->buf, snmpSz+12);
	
	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
	
	bo_snmp_unlock_mut();
}
#endif


/**
 * active_TxRx - Цикл обмена данными по сети RS485.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @msg:
 * @return  0- успех, -1 неудача.
 *
 */
int active_TxRx(struct chan_thread_arg *targ, int dst, char *msg)
{
	int res;
	int rxer = 0;
	int i;
	int n = 1;    /** При использовании n<<i в цикле for имеем
		       * ряд 1,2,4.. для последующего увеличения
		       * времени таймаута при приеме данных по каналу RS485 */
	
	for (i=0; i<targ->nretries; i++) {
		
		/** Передача */
		res = tx(targ, &txBuf, msg);
		if (res < 0) return -1;
		
		/** Прием */
		res = rx(targ, &rxBuf, targ->tout*(n<<i), msg);
		if (res < 0) return -1;
		
		if (get_rxFl(&rxBuf) >= RX_DATA_READY) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				/**  */
				return 0;
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("active_TxRx(%s): Cadr Error !",
				       msg);
				rxer = 1;
				break;
			case RX_TIMEOUT:
				/** Текущее устройство
				 * не успело дать ответ. */
				bo_log("active_TxRx(%s): timeout dst= %d",
				       msg, dst);
				rxer = 1;
				break;
			default:
				bo_log("active_TxRx(%s): state ??? fl= %d",
				       msg, get_rxFl(&rxBuf));
				rxer = 1;
				break;
			}
		}
	}
	bo_log("active_TxRx(%s): get_rxFl(&rxBuf)=[%d], rxBuf.buf[1]=[%d], dst=[%d]",
	       msg, get_rxFl(&rxBuf), rxBuf.buf[1], dst);
	
	if (rxer)
		/** Текущее устройство вычеркиваем из списка. */
		remf_rtbl(targ, &dstBuf, dst);
	else if (get_rxFl(&rxBuf) == RX_DATA_READY) {
		return -1;
	}
	
	return 0;
}

/**
 * active_netStat - Запрос конфигурации сети RS485.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 */
int active_netStat(struct chan_thread_arg *targ, int dst)
{
	int res;
	
	prepare_cadr_actNetStat(targ, &txBuf, dst);

	/** Данные для передачи подготовлены */
	res = active_TxRx(targ, dst, "1netStat");
	
	return res;
}


#ifdef __LOG__
/**
 * active_getLog - Запрос лога.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 */
int active_getLog(struct chan_thread_arg *targ, int dst)
{
	int res;

	prepare_cadr_quLog(targ, &txBuf, dst);
	
	/** Данные для передачи подготовлены */
	res = active_TxRx(targ, dst, "1log");
	
	return res;
}
#endif

#ifdef __SNMP__
/**
 * active_snmp - Запрос состояния магистрали.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 */
int active_snmp(struct chan_thread_arg *targ, int dst)
{
	int res;

	prepare_cadr_snmpStat(targ, &txBuf, dst);
	
	/** Данные для передачи подготовлены */
	res = active_TxRx(targ, dst, "1snmp");
	
	return res;
}
#endif

/**
 * active_local - Запросы устройствам локального узла.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 */
int active_local(struct chan_thread_arg *targ, int dst)
{
	int res;
	
	pthread_mutex_lock(&mx_psv);

	put_state(&psvdata_ready, 1);
	put_state(&psvAnsdata_ready, 1);
	
	/** Ожидаем готовности послать кадр
	    пассивному устройствуву */
	while (get_state(&psvdata_ready) == 1)
		pthread_cond_wait(&psvdata, &mx_psv);

	/** Ожидаем ответ от пассивного устройства */
	while (get_state(&psvAnsdata_ready) == 1)
		pthread_cond_wait(&psvAnsdata, &mx_psv);

	if (get_state(&psvAnsdata_ready) == 3) {
		/** Пассивное устройство не дало ответ */
		bo_log("active_local(): not response from pass");
		put_state(&psvAnsdata_ready, 0);
		pthread_mutex_unlock(&mx_psv);
		return 0;
	}
	
	pthread_mutex_unlock(&mx_psv);
	
	/** Данные для передачи подготовлены */
	res = active_TxRx(targ, dst, "1aloc");
	
	return res;
}

/**
 * activeFromFIFO - Ответ от пассивного устройства активному устройству
 *               через стек FIFO.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @return  0- успех, -1 неудача.
 *
 */
int activeFromFIFO(struct chan_thread_arg *targ)
{
	unsigned int dst;
	int res = 0;
	
	if (get_state(&actFIFOdata_ready) == 1) {
		/** Есть данные для передачи ответа активному
		 * устройству от пассивного через FIFO */
		pthread_mutex_lock(&mx_actFIFO);

		prepare_cadr(&txBuf, (char *)getFifo_buf, getFifo_ans);
		
		dst = (unsigned int)txBuf.buf[0];
		/** bo_log("activeFromFIFO(): dst= [%d]", dst); */

		put_state(&actFIFOdata_ready, 0);
		pthread_cond_signal(&actFIFOdata);
		
		pthread_mutex_unlock(&mx_actFIFO);
		
		/** Данные для передачи подготовлены */
		res = active_TxRx(targ, dst, "1fifo");
	}

	return res;
}


/**
 * active_process - Обработка ответов от активных устройств на сети RS485.
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
int active_process(struct chan_thread_arg *targ, int dst)
{
	char key[4];  /** Адрес пассивного у-ва */
	int res = 0;
	
	memset(key, 0, 3);
	/** Адрес пассивного у-ва */
	sprintf(key, "%03d", (unsigned char)rxBuf.buf[0]);

	if ((unsigned char)rxBuf.buf[0] == targ->src) {
		/** Ответ активного устройства
		 * для сетевого контроллера */
		if (rxBuf.wpos > 4)
			if (rxBuf.buf[2] == targ->cdnsId) {
				/** Запрос о составе сети RS485 */
				res = active_netStat(targ, dst);
			
#ifdef __LOG__
			} else if (rxBuf.buf[2] == targ->cdquLogId) {
				/** GetLog */
				res = active_getLog(targ, dst);
#endif
#ifdef __SNMP__
			} else if (rxBuf.buf[2] == targ->cdmsId) {
				/** Запрос о состоянии магистрали */
				res = active_snmp(targ, dst);
#endif
			} else {
				/** ID ???
				    bo_log("active(): ID= [%d]", rxBuf.buf[2]); */
				res = 0;
			}
		else
			res = 0;
		
	} else if (test_bufDst(&dst2Buf, (unsigned char)rxBuf.buf[0]) != -1) {
		if (targ->ch2_enable) {
			/** Кадр сети RS485 (local node) */
			/** Запрос для пассивного устройства загрузить
			 * в лог. */
#ifdef __LOG__
			putLog(&rxBuf);
#endif
			res = active_local(targ, dst);
		} else
			bo_log("active(): key= [%s] ch2 disable", key);
		
	} else {
		pthread_mutex_lock(&mx_rtg);
		
		if (rt_iskey(rtg, key)) {
			/** Кадр сети RS485 (FIFO) */		
			/** Запрос для пассивного ус-ва загрузить в лог. */
#ifdef __LOG__
			putLog(&rxBuf);
#endif
			prepareFIFO(&rxBuf, key, dst);
		
		} else {
			bo_log("active(): key= [%s] ???", key);
		}
	
		pthread_mutex_unlock(&mx_rtg);
	}
	
	return res;
}


/**
 * active - Запросы активным устройствам на сети RS485 (порт 1).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @dst:  Адрес устройства.
 * @return  0- успех, -1 неудача.
 *
 * Если устройство отвечает на запрос в пределах отведенного времени,
 * то см. active_process()
 * Если устройство, не отвечает на запрос, то оно (после определенного
 * количества попыток) удаляется из локальной таблицы маршрутов.
 */
int active(struct chan_thread_arg *targ, int dst)
{
	int res;
	int rxer = 0;
	int i;
	int n = 1;    /** При использовании n<<i в цикле for имеем
		       * ряд 1,2,4.. для последующего увеличения
		       * времени таймаута при приеме данных по каналу RS485 */

	/** printf("active(): \n"); */
	
	prepare_cadr_act(targ, &txBuf, dst);

	/** Данные для передачи подготовлены */
	for (i=0; i<targ->nretries; i++) {
		
		/** Передача */
		res = tx(targ, &txBuf, "1act");
		if (res < 0) return -1;
		
		/** Прием */
		res = rx(targ, &rxBuf, targ->tout*(n<<i), "1act");
		if (res < 0) return -1;
		
		if (get_rxFl(&rxBuf) >= RX_DATA_READY) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				/**  */
				return active_process(targ, dst);
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("active(): Cadr Error !");
				rxer = 1;
				break;
			case RX_TIMEOUT:
				/** Текущее устройство не успело дать ответ. */
				bo_log("active(): timeout dst= %d", dst);
				rxer = 1;
				break;
			default:
				bo_log("active(): state ??? fl= %d",
				       get_rxFl(&rxBuf));
				rxer = 1;
				break;
			}
		}
	}
	bo_log("active(): get_rxFl(&rxBuf)=[%d], rxBuf.buf[1]=[%d], dst=[%d]",
	       get_rxFl(&rxBuf), rxBuf.buf[1], dst);
	
	if (rxer)
		/** Текущее устройство вычеркиваем из списка. */
		remf_rtbl(targ, &dstBuf, dst);
	else if (get_rxFl(&rxBuf) == RX_DATA_READY) {
		return -1;
	}
	
	return 0;
}


/**
 * chan1 - Канал 1 (активные устройства).
 * @arg:  Параметры для потока.
 *
 * Поток выполняет следующие действия:
 * 1) Производит запросы активным устройствам;
 * 2) Опрашивает устройства на порту 1.
 */
void *chan1(void *arg)
{
	struct chan_thread_arg *targ = (struct chan_thread_arg *)arg;
	int count_scan = targ->tscan;
	int fscan = 1;
	int scan_dst = targ->dst_beg;
	int dst;
	int res;
	
	while (1) {
		if (!targ->ch1_enable) {
			usleep(50000);
		}
		
		if (count_scan == targ->tscan) {
			count_scan = 0;
			if (targ->ch1_enable) {
				/** Сканируем устройства порт 1 RS485
				 *  (1 раз в сек) */
#ifdef __PRINT__
				write (1, "chan1: 1000ms\n", 14);
#endif
				if (fscan) {
					fscan = 0;
					while (scan_dst <= targ->dst_end) {
						res = scan(targ,
							   &txBuf,
							   &rxBuf,
							   &dstBuf,
							   scan_dst,
							   "1scan");
					
						if (res < 0) {
							bo_log("chan1: exit");
							pthread_exit(0);
						}
					
						scan_dst++;
					}
					scan_dst = targ->dst_beg;
					dstBuf.rpos = 0;
				} else {
					res = scan(targ,
						   &txBuf,
						   &rxBuf,
						   &dstBuf,
						   scan_dst,
						   "1scan");
					
					if (res < 0) {
						bo_log("chan1: exit");
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
		}

		if (targ->ch1_enable) {
			if (dstBuf.wpos > 0) {
				dst = get_bufDst(&dstBuf);
				/** (1 раз в 50 мсек) */
#ifdef __SNMP__
				if ((dst == targ->snmp_uso) &&
				    (targ->snmp_n > 0 && targ->snmp_n <= SNMP_IP_MAX)) {
					bo_snmp_lock_mut();
					if (bo_snmp_isChange() == 1) {
						bo_snmp_unlock_mut();
						printf("chan1: magistral status\n");
						/** Есть изменения на магистрали */
						res = active_snmp(targ, dst);
					} else {
						bo_snmp_unlock_mut();
						/** Разрешение активным устройствам
						    bo_log("ch1(): activate 1 dst=[%d]", dst); */
						res = active(targ, dst);
					}
				} else {
#endif
					/** Разрешение активным устройствам
					    bo_log("ch1(): activate 2 dst=[%d]", dst); */
					res = active(targ, dst);
#ifdef __SNMP__
				}
#endif
				
				if (res < 0) break;

				/** Ответ через FIFO */
				res = activeFromFIFO(targ);
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

