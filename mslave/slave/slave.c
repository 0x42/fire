/*
 *		Moxa-slave
 *
 * Version:	@(#)slave.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */


#include "slave.h"
#include "ocrc.h"
#include "ort.h"
#include "bo_net.h"
#include "bo_net_master_core_log.h"
#include "bo_cycle_arr.h"
#include "bo_parse.h"
#include "bo_snmp_mng.h"


/**
 * gen_moxa_default_cfg - Создание конфигурационного файла по умолчанию.
 * @cfile: Имя конфигурационного файла.
 *
 */
void gen_moxa_default_cfg(char *cfile)
{
	TOHT *cfg;
	FILE *out;

	if ((out = fopen(cfile, "w")) == NULL) {
		fprintf(stderr, "gen_moxa_default_cfg(): cannot open %s\n", cfile);
	} else {
		cfg = ht_new(0);

		cfg_put(cfg, "moxa:log", "mslave.log");
		cfg_put(cfg, "moxa:log_old", "mslave-old.log");

		/** Базовый таймер 50ms.
		 * Таймер активных устройств сети RS485 
		 * Tusec(50000us)= 50ms */
		cfg_put(cfg, "moxa:Tsec", "0");
		cfg_put(cfg, "moxa:Tusec", "50000");
	
		/** Таймер сканирования устройств на сети RS485 
		 * Tusec*Tscan= 1000ms */
		cfg_put(cfg, "moxa:Tscan", "20");

		/** Таймаут приема кадра на сети RS485 
		 * Tout*100us= 20ms */
		cfg_put(cfg, "moxa:Tout", "200");

		/** Число попыток передачи кадра по сети RS485 */
		cfg_put(cfg, "moxa:nRetries", "3");

		/** Цикл WatchDog */
		cfg_put(cfg, "WDT:Tsec", "10");
		cfg_put(cfg, "WDT:Tusec", "0");
		/** WatchDog таймер */
		cfg_put(cfg, "WDT:Tms", "30000");
		/** Разрешение работы WatchDog */
		cfg_put(cfg, "WDT:enable", "0");
		/** Файл для контроля жизни программы через CRON */
		cfg_put(cfg, "WDT:lifeFile", "mslave.life");
	
		/** Moxa slave IP */
		cfg_put(cfg, "eth0:ip", "192.168.100.127");

		/** FIFO server */
		/** номер порта на котором ждем запросы для FIFO */
		cfg_put(cfg, "FIFO:port", "8888");
		/** очередь коннектов */
		cfg_put(cfg, "FIFO:queue_len", "10");
		/** размер очереди */
		cfg_put(cfg, "FIFO:len", "5");
		
		/** RT server IP, port */
		cfg_put(cfg, "RT:sendIp", "192.168.1.127");
		cfg_put(cfg, "RT:sendPort", "8890");
		cfg_put(cfg, "RT:recvIp", "127.0.0.1");
		cfg_put(cfg, "RT:recvPort", "8891");
		
		/** LOGGER server IP, port, максимальное количество строк */
		cfg_put(cfg, "LOGGER:sendIp", "192.168.1.127");
		cfg_put(cfg, "LOGGER:sendPort", "8890");
		cfg_put(cfg, "LOGGER:maxLines", "1024");
	
		/** SNMP */
		/** Массив IP адресов для мониторинга
		 * состояния магистрали */
		cfg_put(cfg, "SNMP:1", "192.168.1.150");
		cfg_put(cfg, "SNMP:2", "192.168.1.151");
		/** Размер массива */
		cfg_put(cfg, "SNMP:n", "2");
		
		/** Параметры серийного порта */
		/** 0: none, 1: odd, 2: even, 3: space, 4: mark */
		cfg_put(cfg, "RS:prmParity", "0");
		/** 5 .. 8 */
		cfg_put(cfg, "RS:prmDatabits", "8");
		/** 1, 2 */
		cfg_put(cfg, "RS:prmStopbit", "1");
		/** 50, 75, 110, 134, 150, 200, 300,
		    600, 1200, 1800, 2400, 4800, 9600,
		    19200, 38400, 57600, 115200, 230400,
		    460800, 500000, 576000, 921600 */
		cfg_put(cfg, "RS:speed", "19200");

		/** Сеть RS485 - 1 (активные устройства) */
		cfg_put(cfg, "RS485_1:port", "1");
		cfg_put(cfg, "RS485_1:adr", "128");
		/** 1- включить, 0- отключить */
		cfg_put(cfg, "RS485_1:enable", "1");

		/** Диапазон адресов */
		cfg_put(cfg, "RS485_1:dstBeg", "229");
		cfg_put(cfg, "RS485_1:dstEnd", "230");

		/** Сеть RS485 - 2 (пассивные устройства) */
		cfg_put(cfg, "RS485_2:port", "2");
		cfg_put(cfg, "RS485_2:adr", "1");
		/** 1- включить, 0- отключить */
		cfg_put(cfg, "RS485_2:enable", "1");

		/** Диапазон адресов */
		cfg_put(cfg, "RS485_2:dstBeg", "2");
		cfg_put(cfg, "RS485_2:dstEnd", "5");

		/** Идентификаторы подсистем ЛС */
		cfg_put(cfg, "LS:gen", "General");         /** 0xC2*/
	
		/** Запросы */
		cfg_put(cfg, "REQ:ag", "AccessGranted");     /** 0xE6 */
		cfg_put(cfg, "REQ:ms", "GetNetworkStatus");  /** 0x29 */
		cfg_put(cfg, "REQ:gl", "GetLog");            /** 0x0E */
		cfg_put(cfg, "REQ:ns", "GetNetRS485Status"); /** ???? */
		
		cfg_put(cfg, "REQ:sq", "StartQuench");       /** 0xC5 */

		cfg_save(cfg, out);
		fclose(out);
	
		cfg_free(cfg);
	}
}

/**
 * init_thrState - Инициализация структуры sta{}.
 * @st: Указатель на структуру sta{}.
 */
void init_thrState(struct sta *st)
{
	pthread_mutex_init(&st->mx, NULL);
	st->state = 0;
}

/**
 * destroy_thrState - Разрушение структуры sta{}.
 * @st: Указатель на структуру sta{}.
 */
void destroy_thrState(struct sta *st)
{
	pthread_mutex_destroy(&st->mx);
}

/**
 * put_state - Установить значение переменной состояния.
 * @st: Указатель на структуру sta{}.
 * @data: Значение.
 */
void put_state(struct sta *st, int data)
{
	pthread_mutex_lock(&st->mx);
	st->state = data;
	pthread_mutex_unlock(&st->mx);
}

/**
 * get_state - Получить значение переменной состояния.
 * @st: Указатель на структуру sta{}.
 * @return  Значение переменной состояния.
 */
int get_state(struct sta *st)
{
	int data;
	pthread_mutex_lock(&st->mx);
	data = st->state;
	pthread_mutex_unlock(&st->mx);
	return data;
}


/**
 * init_thrDstBuf - Инициализация структуры thr_dst_buf{}.
 * @b: Указатель на структуру thr_dst_buf{}.
 */
void init_thrDstBuf(struct thr_dst_buf *b)
{
	pthread_mutex_init(&b->mx, NULL);
	b->rpos = 0;
	b->wpos = 0;
}

/**
 * destroy_thrDstBuf - Разрушение структуры thr_dst_buf{}.
 * @b: Указатель на структуру thr_dst_buf{}.
 */
void destroy_thrDstBuf(struct thr_dst_buf *b)
{
	pthread_mutex_destroy(&b->mx);
}

/**
 * clrFlags_bufDst - Очистить буфер флажков.
 * @b:    Указатель на структуру thr_dst_buf{}.
 */
void clrFlags_bufDst(struct thr_dst_buf *b)
{
	int i;
	
	pthread_mutex_lock(&b->mx);
	
	for (i=0; i<b->wpos; i++)
		b->fbuf[i] = 0;

	pthread_mutex_unlock(&b->mx);
}

/**
 * setFlags_bufDst - Установить все флажки в буфере.
 * @b:    Указатель на структуру thr_dst_buf{}.
 */
void setFlags_bufDst(struct thr_dst_buf *b)
{
	int i;
	
	pthread_mutex_lock(&b->mx);
	
	for (i=0; i<b->wpos; i++)
		b->fbuf[i] = 1;

	pthread_mutex_unlock(&b->mx);
}

/**
 * setFlag_bufDst - Установить флажок в буфере fbuf.
 * @b:   Указатель на структуру thr_dst_buf{}.
 * @pos: Позиция для записи в буфер.
 */
void setFlag_bufDst(struct thr_dst_buf *b, int pos)
{
	pthread_mutex_lock(&b->mx);
	b->fbuf[pos] = 1;
	pthread_mutex_unlock(&b->mx);
}

/**
 * resetFlag_bufDst - Сбросить флажок в буфере fbuf.
 * @b:   Указатель на структуру thr_dst_buf{}.
 * @pos: Позиция для записи в буфер.
 */
void resetFlag_bufDst(struct thr_dst_buf *b, int pos)
{
	pthread_mutex_lock(&b->mx);
	b->fbuf[pos] = 0;
	pthread_mutex_unlock(&b->mx);
}

/**
 * getFlag_bufDst - Получить флажок из буфера fbuf.
 * @b:   Указатель на структуру thr_dst_buf{}.
 * @pos: Позиция флажка в буфере.
 * @return  Значение флажка 0/1.
 */
int getFlag_bufDst(struct thr_dst_buf *b, int pos)
{
	int fl;
	
	pthread_mutex_lock(&b->mx);
	fl = b->fbuf[pos];
	pthread_mutex_unlock(&b->mx);

	return fl;
}

/**
 * test_bufDst - Поиск элемента в буфере.
 * @b:    Указатель на структуру thr_dst_buf{}.
 * @data: Значение для поиска.
 * @return  Позиция элемента в буфере - если присутствует, -1 - нет.
 */
int test_bufDst(struct thr_dst_buf *b, int data)
{
	int i;
	int fl = -1;
	
	pthread_mutex_lock(&b->mx);
	
	for (i=0; i<b->wpos; i++)
		if (b->buf[i] == data) {
			fl = i;
			break;
		}

	pthread_mutex_unlock(&b->mx);

	return fl;
}

/**
 * remf_bufDst - Поиск элемента в буфере.
 * @b:    Указатель на структуру thr_dst_buf{}.
 * @data: Значение для поиска и удаления.
 * @return  1- если нашли и удалили элемент, 0- нет.
 */
int remf_bufDst(struct thr_dst_buf *b, int data)
{
	int i, j;
	int fl = 0;
	
	pthread_mutex_lock(&b->mx);
	
	for (i=0; i<b->wpos; i++)
		if (b->buf[i] == data) {
			b->wpos--;
			for (j=i; j<b->wpos; j++)
				b->buf[j] = b->buf[j+1];
			fl = 1;
			break;
		}

	pthread_mutex_unlock(&b->mx);

	return fl;
}

/**
 * put_bufDst - Записать элемент, если он уникален, в буфер buf по
 *             текущей позиции wpos с последующим инкрементом wpos.
 *             Если значение wpos достигло DST_BUF_SZ, установить
 *             позицию wpos на начало буфера buf.
 * @b:    Указатель на структуру thr_dst_buf{}.
 * @data: Значение для записи в буфер.
 * @return  1- если новая запись, 0- запись уже имеется.
 */
int put_bufDst(struct thr_dst_buf *b, int data)
{
	int i;
	int fl = 1;
	
	pthread_mutex_lock(&b->mx);

	for (i=0; i<b->wpos; i++)
		if (b->buf[i] == data) {
			fl = 0;
			break;
		}

	if (fl) {
		b->buf[b->wpos] = data;
		b->wpos++;
	}
	
	if (b->wpos >= DST_BUF_SZ)
		b->wpos = 0;
	pthread_mutex_unlock(&b->mx);

	return fl;
}

/**
 * get_bufDst - Получить элемент из буфера buf по текущей позиции rpos.
 * @b: Указатель на структуру thr_dst_buf{}.
 * @return  Элемент буфера buf.
 */
int get_bufDst(struct thr_dst_buf *b)
{
	int data;
	pthread_mutex_lock(&b->mx);
	data = b->buf[b->rpos];
	b->rpos++;
	if (b->rpos >= b->wpos)
		b->rpos = 0;
	pthread_mutex_unlock(&b->mx);
	return data;
}


#if defined (__MOXA_TARGET__) && defined (__WDT__)

/**
 * init_thrWdtlife - Инициализация структуры sta{}.
 * @st: Указатель на структуру sta{}.
 */
void init_thrWdtlife(struct sta *st)
{
	pthread_mutex_init(&st->mx, NULL);
	st->state = 0;
}

/**
 * destroy_thrWdtlife - Разрушение структуры sta{}.
 * @st: Указатель на структуру sta{}.
 */
void destroy_thrWdtlife(struct sta *st)
{
	pthread_mutex_destroy(&st->mx);
}

/**
 * set_wdtlife - Установить значение переменной состояния.
 * @st:   Указатель на структуру sta{}.
 * @data: Значение.
 */
void set_wdtlife(struct sta *st, int data)
{
	pthread_mutex_lock(&st->mx);
	st->state = data;
	pthread_mutex_unlock(&st->mx);
}

/**
 * put_wdtlife - Установить значение переменной состояния (старое || новое).
 * @st:   Указатель на структуру sta{}.
 * @data: Значение.
 */
void put_wdtlife(struct sta *st, int data)
{
	pthread_mutex_lock(&st->mx);
	st->state |= data;
	pthread_mutex_unlock(&st->mx);
}

/**
 * get_wdtlife - Получить значение переменной состояния.
 * @st: Указатель на структуру sta{}.
 * @return  Значение переменной состояния.
 */
int get_wdtlife(struct sta *st)
{
	int data;
	pthread_mutex_lock(&st->mx);
	data = st->state;
	pthread_mutex_unlock(&st->mx);
	return data;
}

#endif  /** defined (__MOXA_TARGET__) && defined (__WDT__) */


/**
 * send_rtbl - Послать мастеру всю локальную таблицу маршрутов.
 */
void send_rtbl(char *ip)
{
	int buf[4] = {0};
	char *key;
	unsigned int crc;
	unsigned char cbuf[2] = {0};
	unsigned int dataSize = 23;
	char data[dataSize];
	int i;

	if (rtl == NULL) return;
	str_splitInt(buf, ip, ".");
	for (i=0; i<rt_getsize(rtl); i++) {
		key = rt_getkey(rtl, i);
		if (key == NULL) continue;
		memset(data, 0, dataSize);
		sprintf(data,
			"%03d:%03d.%03d.%03d.%03d:%d",
			atoi(key),
			buf[0],
			buf[1],
			buf[2],
			buf[3],
			rt_getport(rtl, key));

		crc = crc16modbus(data, 21);
		boIntToChar(crc, cbuf);
		data[dataSize-2] = cbuf[0];
		data[dataSize-1] = cbuf[1];
		
		if (bo_sendSetMsg(rtSend_sock, data, dataSize) == -1) {
			bo_log("send_rtbl(): bo_sendSetMsg() %s", "ERROR");
		}
		
		bo_log("send_rtbl(): RT/key= %s", key);
	}
}

/**
 * put_rtbl - Добавление адреса устройства сети RS485 в локальную
 *            таблицу маршрутов.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_dst_buf{}.
 * @dst:  Адрес найденного устройства.
 */
void put_rtbl(struct chan_thread_arg *targ, struct thr_dst_buf *b, int dst)
{
	char key[4];
	char val[18];
	int buf[4];
	unsigned int crc;
	unsigned char cbuf[2] = {0};
	unsigned int dataSize = 23;
	char data[dataSize];
	
	memset(key, 0, 3);
	sprintf(key, "%03d", dst);

	if (put_bufDst(b, dst)) {
		/** Если есть изменения в сети RS485 */
		bo_log("put_rtbl(): dst= %d", dst);

		pthread_mutex_lock(&mx_rtl);
		
		if (!rt_iskey(rtl, key)) {
			/** Если изменения в сети RS485 еще не попали в
			 * локальную таблицу маршрутов, то добавляем запись и
			 * отправляем мастеру изменения */
			memset(val, 0, 17);
			str_splitInt(buf, targ->ip, ".");
			sprintf(val,
				"%03d.%03d.%03d.%03d:%d",
				buf[0],
				buf[1],
				buf[2],
				buf[3],
				targ->port+1);
			rt_put(rtl, key, val);

			/** Установить флажки для загрузки новой
			 * конфигурации сети RS485 */
			setFlags_bufDst(&dstBuf);
			
			if (rtSend_sock > 0) {
				/** Послать мастеру изменения в
				 * таблицу маршрутов */
				memset(data, 0, dataSize);
				sprintf(data,
					"%03d:%03d.%03d.%03d.%03d:%d",
					dst,
					buf[0],
					buf[1],
					buf[2],
					buf[3],
					targ->port+1);

				crc = crc16modbus(data, 21);
				boIntToChar(crc, cbuf);
				data[dataSize-2] = cbuf[0];
				data[dataSize-1] = cbuf[1];

				if (bo_sendSetMsg(rtSend_sock,
						  data,
						  dataSize) == -1) {
					bo_log("put_rtbl(): bo_sendSetMsg() %s",
					       "ERROR");
				}
				
				bo_log("put_rtbl(): RT/dst= %d", dst);
			}
		}
		
		pthread_mutex_unlock(&mx_rtl);
	}
}

/**
 * remf_rtbl - Удаление адреса устройства сети RS485 из локальной
 *            таблицы маршрутов.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_dst_buf{}.
 * @dst:  Адрес удаляемого устройства.
 */
void remf_rtbl(struct chan_thread_arg *targ, struct thr_dst_buf *b, int dst)
{
	char key[4];
	unsigned int crc;
	unsigned char cbuf[2] = {0};
	unsigned int dataSize = 10;
	char data[dataSize];

	memset(key, 0, 3);
	sprintf(key, "%03d", dst);

	if (remf_bufDst(b, dst)) {
		/** Если есть изменения в сети RS485 */
		bo_log("remf_rtbl(): dst= %d", dst);
		
		pthread_mutex_lock(&mx_rtl);
		
		if (rt_iskey(rtl, key)) {
			/** Если изменения в сети RS485 еще не попали в
			 * локальную таблицу маршрутов, то удаляем запись и
			 * отправляем мастеру изменения */
			rt_remove(rtl, key);
			
			/** Установить флажки для загрузки новой
			 * конфигурации сети RS485 */
			setFlags_bufDst(&dstBuf);
			
			if (rtSend_sock > 0) {
				/** Послать мастеру изменения в
				 * таблицу маршрутов */
				memset(data, 0, dataSize);
				sprintf(data, "%03d:%s", dst, "NULL");

				crc = crc16modbus(data, dataSize-2);
			
				boIntToChar(crc, cbuf);
				data[dataSize-2] = cbuf[0];
				data[dataSize-1] = cbuf[1];

				if (bo_sendSetMsg(rtSend_sock,
						  data,
						  dataSize) == -1) {
					bo_log("remf_rtbl(): bo_sendSetMsg() %s",
					       "ERROR");
				}
			
				bo_log("remf_rtbl(): RT/dst= %d", dst);
			}
		}
		
		pthread_mutex_unlock(&mx_rtl);
	}
}

/**
 * putLog - Посылка сообщения мастеру для логирования.
 *          Данные для лога формируются из приемного буфера канала 2
 *          RS485, что является ответом пассивного устройства на
 *          запрос активного устройства. 
 */
void putLog()
{
	unsigned int crc;
	unsigned char cbuf[2] = {0};
	char data[BO_ARR_ITEM_VAL];
	unsigned int dataSize;
	unsigned int ptm;
	int i;

	if (logSend_sock > 0) {
		
		ptm = (unsigned)time(NULL);
		data[0] = ptm & 0xff;
		data[1] = (ptm >> 8) & 0xff;
		data[2] = (ptm >> 16) & 0xff;
		data[3] = (ptm >> 24) & 0xff;
		data[4] = rx2Buf.buf[0];
		data[5] = rx2Buf.buf[1];
		data[6] = rx2Buf.buf[2];
		data[7] = rx2Buf.buf[3];
		data[8] = rx2Buf.buf[4];
		data[9] = rx2Buf.buf[5];
		
		for (i=0; i<rx2Buf.buf[5]; i++)
			data[10+i] = rx2Buf.buf[6+i];
	
		crc = crc16modbus(data, rx2Buf.buf[5]+10);
		
		boIntToChar(crc, cbuf);
		data[rx2Buf.buf[5]+10] = cbuf[0];
		data[rx2Buf.buf[5]+11] = cbuf[1];
	
		dataSize = (unsigned int)(rx2Buf.buf[5]+12);
		
		pthread_mutex_lock(&mx_sendSocket);
		
		if (bo_sendLogMsg(logSend_sock, data, dataSize) == -1) {
			bo_log("putLog(): bo_sendLogMsg() %s", "ERROR");
		}
		
		pthread_mutex_unlock(&mx_sendSocket);
	}
}

/**
 * sendFIFO - Формирование и отправка запроса на сервер FIFO.
 * @key: Адрес устройства в виде ключа глобальной таблицы маршрутов.
 */
void sendFIFO(char *key)
{
	char id[9];
	int i;
	
	pthread_mutex_lock(&mx_rtg);
	pthread_mutex_lock(&mx_dtFIFO);
	/** Получить адрес FIFO сервера из таблицы маршрутов. */
	rt_getip(rtg, key, sfifo.ip);

	if (sfifo.ip[0] != 'N') {
		/** Если устройство существует в глобальной таблице
		 * маршрутов, то сформировать запрос с уникальным
		 * индексом и отправить его на сервер FIFO
		 * соответствующему узлу. */
		if (get_state(&fifodata_ready)) {
			/** Поток send_fifo() готов принять данные для
			 * передачи чужому узлу через стек FIFO */
			
			/** Формирование индекса для кадра посылаемого на
			 * чужой узел (защита от дублирования пакетов) */
			fifo_idx++;
			if (fifo_idx == FIFO_IDX_MAX) fifo_idx = 1;
			
			sprintf(id, "%08d", (fifo_idx & 0xffffffff));
			
			/** Подготовка буфера для FIFO */
			for (i=0; i<8; i++)
				sfifo.buf[i] = id[i];
			
			sfifo.ln = rxBuf.wpos;
			for (i=0; i<sfifo.ln; i++)
				sfifo.buf[i+8] = rxBuf.buf[i];

			sfifo.ln += 8;
			
			/** Кадр данных сформирован - даем сигнал
			 * отправки на чужой узел a_threads.c:
			 * send_fifo(). */
			put_state(&fifodata_ready, 0);
			pthread_cond_signal(&fifodata);
		}
	} else {
		bo_log("sendFIFO(): fifo_ipSend= NULL");
	}
	
	pthread_mutex_unlock(&mx_dtFIFO);
	pthread_mutex_unlock(&mx_rtg);
}


/**
 * prepare_cadr - Формирование кадра для передающего буфера.
 * @b:   Указатель на структуру thr_tx_buf{}.
 * @buf: Указатель на буфер с данными.
 * @ln:  Длина данных.
 */
void prepare_cadr(struct thr_tx_buf *b, char *buf, int ln)
{
	int i;
	
	b->wpos = 0;
	for (i=0; i<ln; i++)
		put_txBuf(b, buf[i]);
}

/**
 * prepare_cadr_scan - Формирование кадра для передающего буфера
 *                     (опрос устройств на сети RS485).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @dst:  Адрес устройства.
 */
void prepare_cadr_scan(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst)
{
	unsigned int crc;
	
	b->wpos = 0;
	put_txBuf(b, (char)dst);
	put_txBuf(b, (char)targ->src);

	crc = crc16modbus(b->buf, 2);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

/**
 * prepare_cadr_act - Формирование кадра для передающего буфера
 *                     (Разрешение активным устройствам на сети RS485).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @dst:  Адрес устройства.
 */
void prepare_cadr_act(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst)
{
	unsigned int crc;

	b->wpos = 0;
	put_txBuf(b, (char)dst);
	put_txBuf(b, (char)targ->src);

	put_txBuf(b, (char)targ->cdaId);
	put_txBuf(b, (char)targ->cdaDest);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)0);

	crc = crc16modbus(b->buf, 6);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

/**
 * prepare_cadr_actNetStat - Формирование кадра для передающего буфера
 *                     (конфигурация устройств на сети RS485).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @dst:  Адрес устройства.
 *
 * adr: 15 -> bitAdr: 1000 0000 0000 0000
 *                    |                 |
 *                   15                 0
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
	put_txBuf(b, (char)targ->cdnsDest);
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
		abit[31-(uadr/8)] |= (unsigned char)(1 << (uadr % 8));
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
			abit[31-(uadr/8)] |= (unsigned char)(1 << (uadr % 8));
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


/**
 * prepare_cadr_quLog - Формирование кадра для передающего буфера
 *                     (запрос лога).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @dst:  Адрес устройства.
 */
void prepare_cadr_quLog(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst)
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
		put_txBuf(b, (char)targ->cdquDest);
		put_txBuf(b, (char)0);

		put_txBuf(b, (char)4);

		put_txBuf(b, rxBuf.buf[6]);
		put_txBuf(b, rxBuf.buf[7]);
		put_txBuf(b, rxBuf.buf[8]);
		put_txBuf(b, rxBuf.buf[9]);
		
		/** Ограничиваем запрашиваемый номер строки лога
		    nLogLine = ((rxBuf.buf[6] & 0xff) |
		    (rxBuf.buf[7] << 8)) & (targ->logMaxLines-1); */
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
				bo_log("prepare_cadr_quLog(): bo_master_core_logRecv ERROR");
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
	put_txBuf(b, (char)targ->cdmsDest);
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

	bo_snmp_lock_mut();
		
	memset(data, 0, 1024);

	o_sw = bo_snmp_get_tab();
	
	snmpSz = 0;
	sprintf(ip, "%d.%d.%d.%d",
		rxBuf.buf[6],
		rxBuf.buf[7],
		rxBuf.buf[8],
		rxBuf.buf[9]);
	
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
		/*
		printf("prepare_cadr_snmpStat: snmpSz=%d\n", snmpSz);
		bo_log("prepare_cadr_snmpStat: snmp=%s", data);
		*/
		
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


/**
 * trx - Передача кадра данных устройствам сети RS485 и
 *       получение от него ответа.
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @tb:
 * @rb:
 * @tout:
 * @return  0- успех, -1 неудача.
 */
int trx(struct chan_thread_arg *targ,
	struct thr_tx_buf *tb,
	struct thr_rx_buf *rb,
	int tout)
{
	char tbuf[BUF485_SZ];  /** Буфер передатчика RS485 */
	char buf[BUF485_SZ];  /** Буфер приемника RS485 */
	int res;

	/** Tx */
	res = writer(tb, tbuf, targ->port);	
	if (res < 0) return -1;

	/** Rx */
	put_rxFl(rb, RX_WAIT);  /** Состояние приема - 'ожидание данных' */
	rb->wpos = 0;  /** Позиция записи в начало буфера приемника */
	res = 1;
	
	while (res) {
		res = reader(rb, buf, targ->port, tout);
		if (res < 0) return -1;
	}
	
	return 0;
}

/**
 * scan - Опрос устройств на сети RS485 (порт 1, 2).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @tb:
 * @rb:
 * @db:
 * @dst:  Адрес устройства.
 * @msg:
 * @return  0- успех, -1 неудача.
 *
 * Опрос производится в пределах заданного диапазона адресов,
 * прописанного в конфигурационном файле.
 * Если устройство отвечает на запрос в пределах отведенного времени, то
 * его адрес прописывается в локальной таблице маршрутов.
 * Если устройство, не отвечает на запрос, то оно удаляется из
 * локальной таблицы маршрутов.
 */
int scan(struct chan_thread_arg *targ,
	 struct thr_tx_buf *tb,
	 struct thr_rx_buf *rb,
	 struct thr_dst_buf *db,
	 int dst,
	 char *msg)
{
	int res;
#ifdef __PRINT__
	char a[4];
#endif

	prepare_cadr_scan(targ, tb, dst);

	/** Данные для передачи подготовлены */
	res = trx(targ, tb, rb, targ->tout);
	if (res < 0) return -1;
	
	if ((get_rxFl(rb) >= RX_DATA_READY) &&
	    ((rb->buf[1] & 0xFF) == dst)) {
		switch (get_rxFl(rb)) {
		case RX_DATA_READY:
			put_rtbl(targ, db, dst);
#ifdef __PRINT__
			memset(a, 0, 3);		
			sprintf(a, "%d", dst);
			write(1, a, 4);
#endif
			break;
		case RX_ERROR:
			/** Ошибка кадра */
			bo_log("scan(%s): Cadr Error !", msg);
			break;
		case RX_TIMEOUT:
			/** Текущее устройство не отвечает,
			 * вычеркиваем его из списка. */
			bo_log("scan(%s): timeout dst= %d", msg, dst);
			remf_rtbl(targ, db, dst);
#ifdef __PRINT__
			write(1, ".", 1);
#endif
			break;
		default:
			bo_log("scan(%s): state ??? fl= %d", msg, get_rxFl(rb));
			break;
		}
	} else {
#ifdef __PRINT__
		write(1, "-", 1);
#endif
	}

	return 0;
}
