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
 * trx2 - Передача кадра данных пассивному устройству сети RS485 и
 *        получение от него ответа.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @return  0- успех, -1 неудача.
 */
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

/**
 * scan2 - Опрос устройств на сети RS485 (порт 2).
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
int scan2(struct chan_thread_arg *targ, int dst)
{
	int res;
#ifdef __PRINT__
	char a[4];
#endif

	prepare_cadr_scan(targ, &tx2Buf, dst);
	
	/** Данные для передачи подготовлены */
	res = trx2(targ);
	if (res < 0) return -1;
	
	if ((get_rxFl(&rx2Buf) >= RX_DATA_READY) &&
	    ((rx2Buf.buf[1] & 0xFF) == dst)) {
		switch (get_rxFl(&rx2Buf)) {
		case RX_DATA_READY:
			put_rtbl(targ, &dst2Buf, dst);
#ifdef __PRINT__
			memset(a, 0, 3);
			sprintf(a, "%d", dst);
			write(1, a, 4);
#endif
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
#ifdef __PRINT__
			write(1, ",", 1);
#endif
			break;
		default:
			bo_log("scan2(): state ??? fl= %d", get_rxFl(&rx2Buf));
			break;
		}
	} else {
#ifdef __PRINT__
		write(1, "_", 1);
#endif
	}

	return 0;
}


/**
 * send_dataToPassive - Запросы пассивным устройствам на сети RS485 (порт 2).
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
int send_dataToPassive(struct chan_thread_arg *targ)
{
	int nretr = targ->nretries;
	unsigned char buf[BO_FIFO_ITEM_VAL];
	int bufSize = BO_FIFO_ITEM_VAL;
	int dst;
	int ans;
	int res;
	
	if (get_state(&psvdata_ready) == 1) {
		/** Есть данные для передачи пассивному устройству */
		pthread_mutex_lock(&mx_psv);

		dst = (unsigned int)rxBuf.buf[0];

		prepare_cadr(&tx2Buf, rxBuf.buf, rxBuf.wpos);
			
		put_state(&psvdata_ready, 0);
		pthread_cond_signal(&psvdata);
		
		pthread_mutex_unlock(&mx_psv);
		
	} else {
		ans = bo_getFifoVal(buf, bufSize);
		if (ans > 0) {
			/** В FIFO есть данные для передачи пассивному
			 * устройству */
			dst = (unsigned int)buf[0];

			prepare_cadr(&tx2Buf, (char *)buf, ans);

		} else {
			/** Если в FIFO нет данных */
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


/**
 * chan2 - Канал 2 (пассивные устройства).
 * @arg:  Параметры для потока.
 *
 * Поток выполняет следующие действия:
 * 1) Организует таймеры 50ms и 1000ms;
 * 2) По таймеру 50ms производит запросы пассивным устройствам;
 * 3) По таймеру 1000ms опрашивает устройства на порту 2.
 */
void *chan2(void *arg)
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
			if (targ->ch2_enable) {
				/** Сканируем устройства порт 2 RS485
				 *  (1 раз в сек) */
#ifdef __PRINT__
				write (1, "chan2: 1000ms\n", 14);
#endif
				dst = targ->dst_beg;
				while (dst <= targ->dst_end) {
					res = scan2(targ, dst);
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
			if (dst2Buf.wpos > 0)
				/** Если пассивные устройства имеются
				 * на шине, то разрешаем проверить
				 * наличие запросов к ним. */
				res = send_dataToPassive(targ);
			
			if (res < 0) break;
		}
		
#if defined (__MOXA_TARGET__) && defined (__WDT__)
			if (targ->wdt_en)
				put_wdtlife(&wdt_life, WDT_CHAN2);
#endif
		
		count_scan++;
	}
	
	bo_log("chan2: exit");
	pthread_exit(0);
}

