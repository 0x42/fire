/*
 *		Обработка кадра данных (канал RS485).
 *
 * Version:	@(#)ocs.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#include "ocs.h"
#include "ocrc.h"
#include "bologging.h"
#include "serial.h"


/** Инициализация структуры thr_rx_buf{} */
void init_thrRxBuf(struct thr_rx_buf *b)
{
	pthread_mutex_init(&b->mx, NULL);
	pthread_cond_init(&b->empty, NULL);
	pthread_cond_init(&b->full, NULL);
	b->rpos = 0;
	b->wpos = 0;
	b->fl = 0;
}

/** Разрушение структуры thr_rx_buf{} */
void destroy_thrRxBuf(struct thr_rx_buf *b)
{
	pthread_mutex_destroy(&b->mx);
	pthread_cond_destroy(&b->empty);
	pthread_cond_destroy(&b->full);
}

/** Получить элемент из буфера buf по текущей позиции rpos */
char get_rxBuf(struct thr_rx_buf *b)
{
	char data;
	pthread_mutex_lock(&b->mx);
	data = b->buf[b->rpos];
	b->rpos++;
	pthread_mutex_unlock(&b->mx);
	return data;
}

/** Записать элемент в буфер buf по текущей позиции wpos */
void put_rxBuf(struct thr_rx_buf *b, char data)
{
	pthread_mutex_lock(&b->mx);
	b->buf[b->wpos] = data;
	b->wpos++;
	pthread_mutex_unlock(&b->mx);
}

/** Получить fl */
int get_rxFl(struct thr_rx_buf *b)
{
	int fl;
	pthread_mutex_lock(&b->mx);
	fl = b->fl;
	pthread_mutex_unlock(&b->mx);
	return fl;
}

/** Записать fl */
void put_rxFl(struct thr_rx_buf *b, int fl)
{
	pthread_mutex_lock(&b->mx);
	b->fl = fl;
	pthread_mutex_unlock(&b->mx);
}


/** Инициализация структуры thr_tx_buf{} */
void init_thrTxBuf(struct thr_tx_buf *b)
{
	pthread_mutex_init(&b->mx, NULL);
	pthread_cond_init(&b->empty, NULL);
	pthread_cond_init(&b->full, NULL);
	b->rpos = 0;
	b->wpos = 0;
}

/** Разрушение структуры thr_tx_buf{} */
void destroy_thrTxBuf(struct thr_tx_buf *b)
{
	pthread_mutex_destroy(&b->mx);
	pthread_cond_destroy(&b->empty);
	pthread_cond_destroy(&b->full);
}

/** Получить элемент из буфера buf по текущей позиции rpos */
char get_txBuf(struct thr_tx_buf *b)
{
	char data;
	pthread_mutex_lock(&b->mx);
	data = b->buf[b->rpos];	
	b->rpos++;
	pthread_mutex_unlock(&b->mx);
	return data;
}

/** Записать элемент в буфер buf по текущей позиции wpos */
void put_txBuf(struct thr_tx_buf *b, char data)
{
	pthread_mutex_lock(&b->mx);
	b->buf[b->wpos] = data;
	b->wpos++;
	pthread_mutex_unlock(&b->mx);
}


/**
 * read_byte - Чтение 1 байта по каналу RS485.
 * @b:    Указатель на структуру thr_rx_buf{} (ocs.h).
 * @data: Байт данных кадра, полученный приемником RS485.
 * @fl:   Флаг статуса приема 1 кадра
 *        0- ожидание данных,
 *        1- прием данных,
 *        2- получили ESC(0xDB),
 *        3- конец данных,
 *        4- кадр поврежден.
 *        5- таймаут reader().
 * @return  Состояние флага статуса.
 */
int read_byte(struct thr_rx_buf *b, char data, int fl)
{
	unsigned int crc, icrc;

	/* bo_log("reader: data= %d", (unsigned int)data); */
	
	switch (fl) {
	case 0:
		if (data == '\xFF') {
			/** Получили Sync */
			put_rxFl(b, 0);
		} else if (data == '\xC0') {
			/** Начало приема данных */
			b->wpos = 0;
			put_rxFl(b, 1);
		}
		break;
	case 1:  /** Прием данных */
		if (data == '\xDB') {
			/** Разбор stuffing байтов */
			put_rxFl(b, 2);
			break;
		} else if (data == '\xC0') {
			if (b->wpos >= 4) {
				/** Конец приема данных */
				/** Подсчет и проверка CRC */
				icrc = (((b->buf[b->wpos-1] & 0xFF) << 8) |
					(b->buf[b->wpos-2] & 0xFF)) & 0xFFFF;
				crc = crc16modbus(b->buf, b->wpos-2);
				if (icrc != crc) {
					/** Ошибка CRC */
					bo_log("reader: icrc= %d, crc= %d", icrc, crc);
					bo_log("reader crc: wpos= %d", b->wpos);
					b->wpos = 0;
					put_rxFl(b, 4);
				} else {
					/** Обработка кадра */
					put_rxFl(b, 3);
				}
				break;
			} else {
				/** Возможно это начало, а не конец данных */
				b->wpos = 0;
				put_rxFl(b, 1);
				break;
			}
		} else
			put_rxBuf(b, data);
		break;
	case 2:  /** Разбор stuffing байтов */
		if (data == '\xDC') data = '\xC0';
		else if (data == '\xDD') data = '\xDB';
		else {
			/** Кадр поврежден
			bo_log("reader: data= %d", (unsigned int)data); */
			b->wpos = 0;
			put_rxFl(b, 4);
			break;
		}
		put_rxBuf(b, data);
		put_rxFl(b, 1);
		break;
	}

	return get_rxFl(b);
}

/**
 * reader - Чтение байт по каналу RS485 и первичная обработка данных.
 * @b:     Указатель на структуру thr_rx_buf{} (ocs.h).
 * @buf:   Указатель на буфер приемника RS485.
 * @port:  Порт RS485.
 * @ptout: Предустановка таймаута приема кадра.
 * @return  0: успешно приняли кадр,
 *          1: кадр принят не полностью,
 *         -1: не успех.
 */
int reader(struct thr_rx_buf *b, char *buf, int port, int ptout)
{
	int nq;       /** Кол-во байт проверенных функцией
		       * SerialDataInInputQueue() */
	int n;        /** Кол-во байт принятых функцией
		       * SerialNonBlockRead() */
	int tout = 0;
	int i;
	
	while (tout < ptout) {  /** default: 20ms */
		tout++;
		usleep(100);
		nq = SerialDataInInputQueue(port);
		if (nq > 1) {
			tout = 0;
			break;
		} else if (nq < 0) {
			bo_log("trx: nq <= 0 exit");
			return -1;
		}
	}
	
	if (tout > 0) {
		/** Ответ не получен за допустимый период
		    bo_log("rx: timeout"); */
		put_rxFl(b, 5);
		
	} else {
		n = SerialNonBlockRead(port, buf, BUF485_SZ);
		if (n < 0) {
			bo_log("trx: SerialNonBlockRead exit");
			return -1;
		}

		for (i=0; i<n; i++) {
			put_rxFl(b, read_byte(b, buf[i], get_rxFl(b)));
			if (get_rxFl(b) >= 3)
				break;
		}
	}
	
	if (get_rxFl(b) >= 3) {
		/** Данные приняты */
		return 0;
	}

	return 1;
}


/**
 * prepare_buf_tx - Подготовка данных для передачи по каналу RS485.
 * @b:   Указатель на структуру thr_tx_buf{} (ocs.h).
 * @buf: Указатель на буфер передатчика RS485.
 * @return  Кол-во байт подготовленных для передачи.
 */
int prepare_buf_tx(struct thr_tx_buf *b, char *buf)
{
	char data;
	int len = b->wpos;  /** Кол-во байт в кадре */
	int n = 0;

	b->rpos = 0;
	
	while (len--) {
		data = get_txBuf(b);
		if ((data == '\xC0') &&
		    ((n > 1) && (n < b->wpos-1))) {
			buf[n++] = '\xDB';
			buf[n++] = '\xDC';
		} else if (data == '\xDB') {
			buf[n++] = '\xDB';
			buf[n++] = '\xDD';
		} else
			buf[n++] = data;
	}
	
	return n;
}

/**
 * writer - Передача кадра данных по каналу RS485.
 * @b:    Указатель на структуру thr_tx_buf{} (ocs.h).
 * @buf:  Указатель на буфер передатчика RS485.
 * @port: Порт RS485.
 * @return  0: успешно передали кадр,
 *         -1: не успех.
*/
int writer(struct thr_tx_buf *b, char *buf, int port)
{
	int n;  /** Кол-во байт подготовленных для передачи */

	n = prepare_buf_tx(b, buf);

	if (SerialWrite(port, buf, n) <= 0) {
		bo_log("writer: SerialWrite exit");
		return -1;
	}

	return 0;
}

