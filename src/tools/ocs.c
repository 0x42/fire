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


/**
 * init_thrRxBuf - Инициализация структуры thr_rx_buf{}.
 * @b: Указатель на структуру thr_rx_buf{}.
 */
void init_thrRxBuf(struct thr_rx_buf *b)
{
	pthread_mutex_init(&b->mx, NULL);
	pthread_cond_init(&b->empty, NULL);
	pthread_cond_init(&b->full, NULL);
	b->rpos = 0;
	b->wpos = 0;
	b->fl = 0;
}

/**
 * destroy_thrRxBuf - Разрушение структуры thr_rx_buf{}.
 * @b: Указатель на структуру thr_rx_buf{}.
 */
void destroy_thrRxBuf(struct thr_rx_buf *b)
{
	pthread_mutex_destroy(&b->mx);
	pthread_cond_destroy(&b->empty);
	pthread_cond_destroy(&b->full);
}

/**
 * get_rxBuf - Получить элемент из буфера buf по текущей позиции rpos.
 * @b: Указатель на структуру thr_rx_buf{}.
 * @return: Байт данных кадра.
 */
char get_rxBuf(struct thr_rx_buf *b)
{
	char data;
	pthread_mutex_lock(&b->mx);
	data = b->buf[b->rpos];
	b->rpos++;
	pthread_mutex_unlock(&b->mx);
	return data;
}

/**
 * put_rxBuf - Записать элемент в буфер buf по текущей позиции wpos.
 * @b:    Указатель на структуру thr_rx_buf{}.
 * @data: Байт данных кадра.
 */
void put_rxBuf(struct thr_rx_buf *b, char data)
{
	pthread_mutex_lock(&b->mx);
	b->buf[b->wpos] = data;
	b->wpos++;
	pthread_mutex_unlock(&b->mx);
}

/**
 * get_rxFl - Получить значение флага состояния приема кадра.
 * @b: Указатель на структуру thr_rx_buf{}.
 * @return  Состояние флага.
 */
int get_rxFl(struct thr_rx_buf *b)
{
	int fl;
	pthread_mutex_lock(&b->mx);
	fl = b->fl;
	pthread_mutex_unlock(&b->mx);
	return fl;
}

/**
 * put_rxFl - Записать значение флага состояния приема кадра.
 * @b:  Указатель на структуру thr_rx_buf{}.
 * @fl: Флаг состояния приема кадра.
 */
void put_rxFl(struct thr_rx_buf *b, int fl)
{
	pthread_mutex_lock(&b->mx);
	b->fl = fl;
	pthread_mutex_unlock(&b->mx);
}


/**
 * init_thrTxBuf - Инициализация структуры thr_tx_buf{}.
 * @b: Указатель на структуру thr_tx_buf{}.
 */
void init_thrTxBuf(struct thr_tx_buf *b)
{
	pthread_mutex_init(&b->mx, NULL);
	pthread_cond_init(&b->empty, NULL);
	pthread_cond_init(&b->full, NULL);
	b->rpos = 0;
	b->wpos = 0;
}

/**
 * destroy_thrTxBuf - Разрушение структуры thr_tx_buf{}.
 * @b: Указатель на структуру thr_tx_buf{}.
 */
void destroy_thrTxBuf(struct thr_tx_buf *b)
{
	pthread_mutex_destroy(&b->mx);
	pthread_cond_destroy(&b->empty);
	pthread_cond_destroy(&b->full);
}

/**
 * get_txBuf - Получить элемент из буфера buf по текущей позиции rpos с
 *             последующим инкрементом rpos.
 * @b: Указатель на структуру thr_tx_buf{}.
 * @return: Байт данных кадра.
 */
char get_txBuf(struct thr_tx_buf *b)
{
	char data;
	pthread_mutex_lock(&b->mx);
	data = b->buf[b->rpos];	
	b->rpos++;
	pthread_mutex_unlock(&b->mx);
	return data;
}

/**
 * put_txBuf - Записать элемент в буфер buf по текущей позиции wpos с
 *             последующим инкрементом wpos.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @data: Байт данных кадра.
 */
void put_txBuf(struct thr_tx_buf *b, char data)
{
	pthread_mutex_lock(&b->mx);
	b->buf[b->wpos] = data;
	b->wpos++;
	pthread_mutex_unlock(&b->mx);
}

/**
 * set_txBuf - Записать элемент в буфер buf.
 * @b:    Указатель на структуру thr_tx_buf{}.
 * @pos:  Позиция для записи в буфер.
 * @data: Байт данных кадра.
 */
void set_txBuf(struct thr_tx_buf *b, int pos, char data)
{
	pthread_mutex_lock(&b->mx);
	b->buf[pos] = data;
	pthread_mutex_unlock(&b->mx);
}


/**
 * read_byte - Чтение 1 байта по каналу RS485.
 * @b:    Указатель на структуру thr_rx_buf{} (ocs.h).
 * @data: Байт данных кадра, полученный приемником RS485.
 * @fl:   Флаг статуса приема 1 кадра.
 * @return  Состояние флага статуса.
 */
int read_byte(struct thr_rx_buf *b, char data, int fl)
{
	unsigned int icrc;  /** Принятая контрольная сумма */
	unsigned int crc;   /** Подсчитанная контрольная сумма */
	
	switch (fl) {
	case RX_WAIT:
		if (data == '\xFF') {
			/** Получили Sync */
			put_rxFl(b, RX_WAIT);
		} else if (data == '\xC0') {
			/** Начало приема данных */
			b->wpos = 0;
			put_rxFl(b, RX_READ);
		}
		break;
		
	case RX_READ:  /** Прием данных */
		if (data == '\xDB') {
			/** Разбор stuffing байтов */
			put_rxFl(b, RX_ESC);
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
					bo_log("read_byte: icrc= %d, crc= %d",
					       icrc, crc);
					
					b->wpos = 0;
					put_rxFl(b, RX_ERROR);
				} else {
					/** Обработка кадра */
					put_rxFl(b, RX_DATA_READY);
				}
				break;
			} else {
				/** Возможно это начало, а не конец данных */
				b->wpos = 0;
				put_rxFl(b, RX_READ);
				break;
			}
		} else
			put_rxBuf(b, data);
		break;
		
	case RX_ESC:  /** Разбор stuffing байтов */
		if (data == '\xDC') data = '\xC0';
		else if (data == '\xDD') data = '\xDB';
		else {
			/** Кадр поврежден */
			b->wpos = 0;
			put_rxFl(b, RX_ERROR);
			break;
		}
		put_rxBuf(b, data);
		put_rxFl(b, RX_READ);
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
	fd_set set;
	struct timeval timeout;
	int n;
	int sel;
	int i;
	int err;
	int fd = FindFD(port);
	
	FD_ZERO(&set); /* clear the set */
	FD_SET(fd, &set); /* add our file descriptor to the set */
		
	timeout.tv_sec = 0;
	timeout.tv_usec = 1000 * ptout;
		
	sel = select(fd + 1, &set, NULL, NULL, &timeout);
	if(sel < 0)
		return -1;  /* an error accured */
	else if (sel == 0) {
		/** Ответ не получен за допустимый период */
		put_rxFl(b, RX_TIMEOUT);
		return 0;
	} else {
		n = read(fd, buf, BUF485_SZ); /* there was data to read */
		if (n < 0) {
			err = errno;
			bo_log("reader: read exit [%s]",
			       strerror(err));
			return -1;
		} else if (n == 0) {
			bo_log("reader: read exit n == 0");
			return -1;
		} else
			for (i=0; i<n; i++) {
				put_rxFl(b, read_byte(b, buf[i], get_rxFl(b)));
				if (get_rxFl(b) >= RX_DATA_READY) {
					/** Данные приняты */
					return 0;
				}
			}
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
	int len = b->wpos;  /** Кол-во байт в буфере b->buf */
	int n = 0;

	b->rpos = 0;
	buf[n++] = '\xFF';
	buf[n++] = '\xC0';
	
	while (len--) {
		data = get_txBuf(b);
		if (data == '\xC0') {
			buf[n++] = '\xDB';
			buf[n++] = '\xDC';
		} else if (data == '\xDB') {
			buf[n++] = '\xDB';
			buf[n++] = '\xDD';
		} else
			buf[n++] = data;
	}

	buf[n++] = '\xC0';
	
	return n;
}

/**
 * writer - Передача кадра данных по каналу RS485.
 * @b:    Указатель на структуру thr_tx_buf{} (ocs.h).
 * @buf:  Указатель на буфер передатчика RS485.
 * @port: Порт RS485.
 * @return  длина перед. данных / -1: не успех.
int writer(struct thr_tx_buf *b, char *buf, int port)
{
	int n;  // Кол-во байт подготовленных для передачи /
	int res;
	
	n = prepare_buf_tx(b, buf);

	res = SerialWrite(port, buf, n);

	if (res < 0) {
		bo_log("writer: SerialWrite exit");
		return -1;
	}

	if (res != n) {
		bo_log("writer: res= [%d] n= [%d]", res, n);
	}
	
	return res;
}
 */

/**
 * writer - Передача кадра данных по каналу RS485.
 * @b:    Указатель на структуру thr_tx_buf{} (ocs.h).
 * @buf:  Указатель на буфер передатчика RS485.
 * @port: Порт RS485.
 * @return  длина перед. данных / -1: не успех.
 */
int writer(struct thr_tx_buf *b, char *buf, int port)
{
	int n;  /** Кол-во байт подготовленных для передачи */
	int res;
	int i = 0;
	
	n = prepare_buf_tx(b, buf);
	
	while (i != n) {
		res = SerialWrite(port, buf+i, n-i);
		if (res < 0) {
			bo_log("writer: SerialWrite exit");
			return -1;
		}
		
		i += res;
	}
	
	return res;
}

