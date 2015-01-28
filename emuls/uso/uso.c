
#include "uso.h"
#include "ocrc.h"
#include "ort.h"
#include "ocs.h"
#include "bologging.h"
#include "serial.h"


void gen_uso_default_cfg(char *cfile)
{
	TOHT *cfg;
	FILE *out;
	out = fopen(cfile, "w");
	
	cfg = ht_new(0);

	cfg_put(cfg, "log:log", "uso.log");
	cfg_put(cfg, "log:log_old", "uso-old.log");

	/** Параметры серийного порта */
	cfg_put(cfg, "RS:port", "1");
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
	cfg_put(cfg, "RS:tout", "5");

	cfg_put(cfg, "USO:adr", "229");
	cfg_put(cfg, "USO:logger", "0");
	cfg_put(cfg, "USO:snmp", "0");

	cfg_put(cfg, "LOGGER:line", "1");
	cfg_put(cfg, "LOGGER:nlines", "4");

	/** Идентификаторы подсистем ЛС */
	cfg_put(cfg, "LS:gen", "General");           /** 0xC2*/

	/** Запросы */
	cfg_put(cfg, "REQ:ag", "AccessGranted");     /** 0xE6 */
	cfg_put(cfg, "REQ:ms", "GetMagStatus");      /** ???? */
	cfg_put(cfg, "REQ:gl", "GetLog");            /** 0x0E */
	cfg_put(cfg, "REQ:ns", "GetNetworkStatus");  /** 0x29 */

	cfg_put(cfg, "REQ:sq", "StartQuench");       /** 0xC5 */

	cfg_put(cfg, "PR:adr", "2");

	/** Длина данных сообщения (начальная) */
	cfg_put(cfg, "TEST:ln", "10");
	/** Количество сообщений */
	cfg_put(cfg, "TEST:m", "100");
	/** Количество блоков сообщений, если 0- бесконечно долго */
	cfg_put(cfg, "TEST:nm", "0");
	/** Длина сообщения */
	cfg_put(cfg, "TEST:msg_ln", "9");
	/** Сообщение */
	cfg_put(cfg, "TEST:msg", "USO(229)-");

	/** Тест SNMP */
	cfg_put(cfg, "SNMP:ip", "192.168.1.150");
	
	cfg_save(cfg, out);
	fclose(out);
	
	cfg_free(cfg);
}

int tx(struct actx_thread_arg *targ)
{
	char buf[BUF485_SZ];  /** Буфер передатчика RS485 */
	int res;
	/**
	int i;

	printf("tx: ");
	for (i=0; i<8; i++) {
		// bo_log("rx_485: buf[i]= %d", (unsigned int)buf[i]); /
		printf("0x%02x ", (unsigned char)txBuf.buf[i]);
	}
	printf("\n");
	*/
	
	res = writer(&txBuf, buf, targ->port);	
	if (res < 0) return -1;
	
	return 0;
}

/**
int rx_uso(struct actx_thread_arg *targ)
{
	char buf[BUF485_SZ];  // Буфер приемника RS485 /
	int res;

	put_rxFl(&rxBuf, RX_WAIT);
	rxBuf.wpos = 0;
	res = 1;
	
	while (res) {
		res = reader(&rxBuf, buf, targ->port, targ->tout);
		if (res < 0) return -1;
	}
	
	return 0;
}
*/

int rx(struct actx_thread_arg *targ)
{
	char buf[BUF485_SZ];  /** Буфер приемника RS485 */
	int i;
	int n;        /** Кол-во байт принятых функцией SerialBlockRead() */
	
	/* write(1, "rx:\n", 4); */

	put_rxFl(&rxBuf, RX_WAIT);
	rxBuf.wpos = 0;

	while (1) {
		n = SerialBlockRead(targ->port, buf, BUF485_SZ);

		if (n < 0) {
			bo_log("rx_485: SerialBlockRead exit");
			return -1;
		}
		
		/* printf("rx: "); */
		for (i=0; i<n; i++) {
			/**
			   bo_log("rx_485: buf[i]= %d", (unsigned int)buf[i]);
			   /  /
			
			
			printf("0x%02x ", (unsigned char)buf[i]);
			*/
			put_rxFl(&rxBuf,
				 read_byte(&rxBuf, buf[i], get_rxFl(&rxBuf)));

			if (get_rxFl(&rxBuf) >= RX_DATA_READY) break;		
		}
		/* printf("\n"); */
		
		if (get_rxFl(&rxBuf) >= RX_DATA_READY) {
			/** Данные приняты */
			break;
		}
	}  /** while */

	return 0;
}


int scan(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;
	int res;
	
	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);  /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */
	
	crc = crc16modbus(b->buf, 2);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
								
	res = tx(targ);
	if (res < 0) return -1;

	return 0;
}

int uso(struct actx_thread_arg *targ,
	struct thr_tx_buf *b,
	unsigned int sch,
	int dst)
{
	unsigned int crc;
	int res;
	int i;
	char csch[10];
	int ln = 8;
	
	sprintf(csch, "%08u", sch);
	
	b->wpos = 0;
	put_txBuf(b, (char)dst);     /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */

	put_txBuf(b, (char)targ->cdsqId);
	put_txBuf(b, (char)targ->cdDest);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)(targ->test_msgln+ln));
	
	for (i=0; i<targ->test_msgln; i++)
		put_txBuf(b, targ->test_msg[i]);

	for (i=0; i<ln; i++)
		put_txBuf(b, csch[i]);

	for (i=0; i<sch; i++)
		put_txBuf(b, '\x01');
	
	crc = crc16modbus(b->buf, targ->test_msgln+ln+sch+6);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));

	bo_log("uso(): sch=%d dst=%d src=%d",
	       sch,
	       (unsigned char)dst,
	       (unsigned char)rxBuf.buf[0]);
	
	res = tx(targ);
	if (res < 0) return -1;

	return 0;
}

int uso_quNetStat(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;
	int res;
	
	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);  /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */

	put_txBuf(b, (char)targ->cdnsId);
	put_txBuf(b, (char)targ->cdDest);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)0);

	crc = crc16modbus(b->buf, 7);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
								
	res = tx(targ);
	if (res < 0) return -1;

	return 0;
}

int uso_quLog(struct actx_thread_arg *targ, struct thr_tx_buf *b,
	       unsigned int line, unsigned int nlines)
{
	unsigned int crc;
	int res;

	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);
	put_txBuf(b, rxBuf.buf[0]);
	put_txBuf(b, (char)targ->cdquLogId);
	put_txBuf(b, (char)targ->cdDest);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)4);
	put_txBuf(b, (char)(line & 0xff));
	put_txBuf(b, (char)((line >> 8) & 0xff));
	put_txBuf(b, (char)(nlines & 0xff));
	put_txBuf(b, (char)((nlines >> 8) & 0xff));

	crc = crc16modbus(b->buf, 10);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
								
	res = tx(targ);
	if (res < 0) return -1;

	return 0;
}

int uso_quSNMP(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;
	int buf[4];
	int res;
	
	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);
	put_txBuf(b, rxBuf.buf[0]);
	put_txBuf(b, (char)targ->cdmsId);
	put_txBuf(b, (char)targ->cdDest);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)4);

	str_splitInt(buf, targ->snmp_ip, ".");
	
	put_txBuf(b, (char)buf[0]);
	put_txBuf(b, (char)buf[1]);
	put_txBuf(b, (char)buf[2]);
	put_txBuf(b, (char)buf[3]);

	crc = crc16modbus(b->buf, 10);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
								
	res = tx(targ);
	if (res < 0) return -1;

	return 0;
}

int uso_answer(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;
	int res;
	
	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);  /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */
	
	put_txBuf(b, 0);  /** */
	put_txBuf(b, 0);  /** */

	crc = crc16modbus(b->buf, 4);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
								
	res = tx(targ);
	if (res < 0) return -1;

	return 0;
}

