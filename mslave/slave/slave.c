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
		cfg_put(cfg, "REQ:ns", "GetNetworkStatus");  /** 0x29 */
		cfg_put(cfg, "REQ:gl", "GetLog");            /** 0x0E */

		cfg_save(cfg, out);
		fclose(out);
	
		cfg_free(cfg);
	}
}

/**
 * gen_moxa_cron_life - Создание файла для контроля жизни программы
 *                      через CRON.
 * @cfile: Имя файла.
 *
void gen_moxa_cron_life(char *cfile)
{
	TOHT *life;
	FILE *out;

	if ((out = fopen(cfile, "w")) == NULL) {
		fprintf(stderr, "gen_moxa_cron_life(): cannot open %s\n", cfile);
	} else {
		life = ht_new(0);

		// Счетчики жизни.
		//    Если значение счетчика втечение периода CRON не
		//    изменится, то контроллер будет перезагружен.
		cfg_put(life, "WDT:sch_life", "0");

		cfg_save(life, out);
		fclose(out);
	
		cfg_free(life);
	}
}
 */

/**
 * inc_cron_life - Увеличивает счетчик жизни программы на 1.
 * @lifile: Имя файла.
 *
 */
void inc_cron_life(char *lifile)
{
	TOHT *life;
	FILE *out;
	char slife[17] = {0};
	int sch_life;

	life = cfg_load(lifile);
	sch_life = cfg_getint(life, "WD:life", -1);
	sprintf(slife, "%016d", ++sch_life);
	
	if ((out = fopen(lifile, "w")) == NULL) {
		fprintf(stderr, "inc_cron_life(): cannot open %s\n", lifile);
	} else {
		cfg_put(life, "WD:life", slife);
		cfg_save(life, out);
		fclose(out);
	}
	
	cfg_free(life);
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
 * test_bufDst - Поиск элемента в буфере.
 * @b:    Указатель на структуру thr_dst_buf{}.
 * @data: Значение для поиска.
 * @return  1- если присутствует, 0- нет.
 */
int test_bufDst(struct thr_dst_buf *b, int data)
{
	int i;
	int fl = 0;
	
	pthread_mutex_lock(&b->mx);
	
	for (i=0; i<b->wpos; i++)
		if (b->buf[i] == data) {
			fl = 1;
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
	/* int i; */
	
	memset(key, 0, 3);
	sprintf(key, "%03d", dst);

	if (put_bufDst(b, dst)) {
		/** Если есть изменения в сети RS485 */
		bo_log("put_rtbl(): dst= %d", dst);
	}

	if (rtSend_sock > 0) {
		
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

			/**
			for (i=0; i<dataSize; i++)
				bo_log("%d", (unsigned char)data[i]);
			*/
			
			if (bo_sendSetMsg(rtSend_sock, data, dataSize) == -1) {
				bo_log("put_rtbl(): bo_sendSetMsg() %s", "ERROR");
			}
			
			bo_log("put_rtbl(): RT/dst= %d", dst);
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
	/* int i; */

	memset(key, 0, 3);
	sprintf(key, "%03d", dst);

	if (remf_bufDst(b, dst)) {
		/** Если есть изменения в сети RS485 */
		bo_log("remf_rtbl(): dst= %d", dst);
	}
	
	if (rtSend_sock > 0) {
		
		pthread_mutex_lock(&mx_rtl);
		
		if (rt_iskey(rtl, key)) {
			/** Если изменения в сети RS485 еще не попали в
			 * локальную таблицу маршрутов, то удаляем запись и
			 * отправляем мастеру изменения */
			rt_remove(rtl, key);
			
			memset(data, 0, dataSize);
			sprintf(data, "%03d:%s", dst, "NULL");

			crc = crc16modbus(data, dataSize-2);
			
			boIntToChar(crc, cbuf);
			data[dataSize-2] = cbuf[0];
			data[dataSize-1] = cbuf[1];

			/**
			for (i=0; i<dataSize; i++)
				bo_log("%d", (unsigned char)data[i]);
			*/
			
			if (bo_sendSetMsg(rtSend_sock, data, dataSize) == -1) {
				bo_log("remf_rtbl(): bo_sendSetMsg() %s", "ERROR");
			}
			
			bo_log("remf_rtbl(): RT/dst= %d", dst);
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
 * @fifo_port: Номер порта открытого сокета FIFO для отправки запроса.
 * @key:       Адрес устройства в виде ключа глобальной таблицы маршрутов.
 */
void sendFIFO(int fifo_port, char *key)
{
	char buf[BUF485_SZ+8];
	char ip[16] = {0};
	char id[9];
	unsigned int ln;
	int i;
	int ans;
	int np;
	
	pthread_mutex_lock(&mx_rtg);
	/** Получить адрес FIFO сервера из таблицы маршрутов. */
	rt_getip(rtg, key, ip);
	
	pthread_mutex_unlock(&mx_rtg);

	if (ip[0] != 'N') {
		/** Если устройство существует в глобальной таблице
		 * маршрутов, то сформировать запрос с уникальным
		 * индексом и отправить его на сервер FIFO
		 * соответствующему узлу. */
		np = 0;
		ans = 0;

		/** Формирование индекса для кадра посылаемого на
		 * чужой узел (защита от дублирования пакетов) */
		fifo_idx++;
		if (fifo_idx == FIFO_IDX_MAX) fifo_idx = 1;
		
		sprintf(id, "%08d", (fifo_idx & 0xffffffff));
		
		/** Подготовка буфера для FIFO */
		ln = rxBuf.wpos;
		for (i=0; i<8; i++)
			buf[i] = id[i];
		
		for (i=0; i<ln; i++)
			buf[i+8] = rxBuf.buf[i];

		while (ans != 1) {
			ans = bo_sendDataFIFO(ip, fifo_port, buf, ln+8);
			usleep(200000);
			np++;
		}
		
		if (np > 1)
			bo_log("sendFIFO(): bo_sendDataFIFO(): np=%d", np);
	} else {
		bo_log("sendFIFO(): fifo_ipSend= NULL");
	}
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

void prepare_cadr_actNetStat(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst)
{
	unsigned int crc;
	int i;
	unsigned int n = 0;
	char *key;
	int apsv[256];
	int psv;

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
		psv = atoi(key);
		if ((psv > 1) && (psv < 128)) {
			/** Получаем список адресов пассивных
			 * устройств на глобальном пространстве. */
			apsv[n] = psv;
			n++;
		}
	}
	pthread_mutex_unlock(&mx_rtg);

	if (n == 0) {
		for (i=0; i<dst2Buf.wpos; i++) {
			apsv[n] = dst2Buf.buf[i];
			n++;
		}
	}
	
	put_txBuf(b, (char)n);
	
	for (i=0; i<n; i++)
		put_txBuf(b, (char)apsv[i]);

	crc = crc16modbus(b->buf, n+6);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

/**
 * intadr_to_bitadr - Преобразование адреса.
 * @dst:  Адрес устройства.
 *
 * adr: 15 -> bitAdr: 1000 0000 0000 0001
 *                    |                 |
 *                   15                 0
unsigned int intadr_to_bitadr(int dst)
{
	unsigned int adr_bit;

	adr_bit = 1 << dst;
	
	return adr_bit;
}
 */

/**
 * prepare_cadr_actNetStat - Формирование кадра для передающего буфера
 *                     (конфигурация устройств на сети RS485).
 * @targ: Указатель на структуру chan_thread_arg{}.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @dst:  Адрес устройства.
void prepare_cadr_actNetStat(struct chan_thread_arg *targ, struct thr_tx_buf *b, int dst)
{
	unsigned int crc;
	int i;
	unsigned int n = 0;
	char *key;
	unsigned char abit[32];
	unsigned int adr_bit;
	int adr;

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
		adr = atoi(key);
		/** Получаем список адресов устройств
		 * на глобальном пространстве.
		adr_bit = 1 << dst;
		
		abit[n] = adr;
		n++;
	}
	pthread_mutex_unlock(&mx_rtg);

	
	if (n == 0) {
		for (i=0; i<dst2Buf.wpos; i++) {
			apsv[n] = dst2Buf.buf[i];
			n++;
		}
	}
	
	put_txBuf(b, (char)n);
	
	for (i=0; i<n; i++)
		put_txBuf(b, (char)apsv[i]);

	crc = crc16modbus(b->buf, n+6);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}
 */


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
			nn = bo_master_core_logRecv(logSend_sock, nLogLine+j, buf, bufSize);
			if (nn == -1) {
				bo_log("prepare_cadr_quLog(): bo_master_core_logRecv ERROR");
				break;
			} else if (nn > 0) {
				logSz += nn;
				for (i=0; i<nn; i++) {
					put_txBuf(b, (char)buf[i]);
				}
			} else
				bo_log("prepare_cadr_quLog(): = 0");			
		}
		
		pthread_mutex_unlock(&mx_sendSocket);

		/** Установить длину сообщения */
		set_txBuf(b, 5, (char)(logSz+4));
		
		crc = crc16modbus(b->buf, logSz+10);

		put_txBuf(b, (char)(crc & 0xff));
		put_txBuf(b, (char)((crc >> 8) & 0xff));
	}
}

