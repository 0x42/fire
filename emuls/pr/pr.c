
#include "pr.h"
#include "ocs.h"
#include "ocfg.h"
#include "ocrc.h"
#include "bologging.h"
#include "serial.h"


void gen_pr_default_cfg(char *cfile)
{
	TOHT *cfg;
	FILE *out;
	out = fopen(cfile, "w");
	
	cfg = ht_new(0);

	cfg_put(cfg, "log:log", "pr.log");
	cfg_put(cfg, "log:log_old", "pr-old.log");

	/** Параметры серийного порта */
	cfg_put(cfg, "RS:port", "2");
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

	cfg_put(cfg, "PR:adr", "3");

	/** Длина сообщения */
	cfg_put(cfg, "TEST:msg_ln", "9");

	cfg_save(cfg, out);
	fclose(out);
	
	cfg_free(cfg);
}

int tx(int port)
{
	char buf[BUF485_SZ];  /** Буфер передатчика RS485 */
	int res;

	/** Tx */
	res = writer(&txBuf, buf, port);
	
	if (res < 0) return -1;
	
	return 0;
}

int rx_pr(int port, int tout)
{
	char buf[BUF485_SZ];  /** Буфер приемника RS485 */
	int res;

	/** Rx */
	put_rxFl(&rxBuf, RX_WAIT);
	rxBuf.wpos = 0;
	res = 1;
	
	while (res) {
		res = reader(&rxBuf, buf, port, tout);
		if (res < 0) return -1;
	}
	
	return 0;
}

int rx(int port)
{
	char buf[BUF485_SZ];  /** Буфер приемника RS485 */
	int i;
	int n;        /** Кол-во байт принятых функцией SerialBlockRead() */
	
	/* write(1, "rx:\n", 4); */

	put_rxFl(&rxBuf, RX_WAIT);
	rxBuf.wpos = 0;

	while (1) {
		n = SerialBlockRead(port, buf, BUF485_SZ);

		if (n < 0) {
			bo_log("rx_485: SerialBlockRead exit");
			return -1;
		}
		
		for (i=0; i<n; i++) {
			/* bo_log("rx_485: buf[i]= %d", (unsigned int)buf[i]);
			   printf("0x%02x ", (unsigned char)buf[i]); */
			
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


void scan(struct thr_tx_buf *b)
{
	unsigned int crc;
	
	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);  /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */
	
	crc = crc16modbus(b->buf, 2);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

void probot(struct thr_tx_buf *b, int msgln)
{
	unsigned int crc;
	unsigned char ln;
	int i;
	char csch[10];
	unsigned int sch;
	int j;
	char tmstr[50] = {0};
	/**
	char data[1200];
	*/
	
	memset(csch, 0, 10);

	ln = (unsigned char)rxBuf.buf[5];
	
	for (i=0; i<ln-msgln; i++)
		csch[i] = (unsigned char)rxBuf.buf[6+i+msgln];
	
	sch = (unsigned int)atoi(csch);

	bo_getTimeNow(tmstr, 50);

	printf("PR[%s]: from active      [%d] ------<<--<<-----\n", tmstr, sch);

	/**
	memset(data, 0, 1200);


	for (j=0; j<ln; j++) {
		data[j] = (unsigned char)rxBuf.buf[6+j];
	}

	for (j=0; j<sch; j++) {
		data[ln+j] = (unsigned char)rxBuf.buf[6+ln+j];
	}

	printf("sch=%d dst=[%d] <- src=[%d] [%02x %02x %02x %02x] [%s]\n",
	       sch,
	       (unsigned char)rxBuf.buf[0],
	       (unsigned char)rxBuf.buf[1],
	       (unsigned char)rxBuf.buf[2],
	       (unsigned char)rxBuf.buf[3],
	       (unsigned char)rxBuf.buf[4],
	       (unsigned char)rxBuf.buf[5],
	       data);
	*/
	
	b->wpos = 0;
	put_txBuf(b, (char)rxBuf.buf[1]);  /** dst */
	put_txBuf(b, (char)rxBuf.buf[0]);  /** src */
	
	put_txBuf(b, (char)rxBuf.buf[2]);  /** Id */
	put_txBuf(b, (char)rxBuf.buf[3]);  /** Dest */
	put_txBuf(b, (char)1);       /** Status */
	
	put_txBuf(b, ln);  /** DataSZ */

	for (i=0; i<ln; i++)
		put_txBuf(b, (char)rxBuf.buf[6+i]);  /** Data */

	for (j=0; j<sch; j++)
		put_txBuf(b, (char)rxBuf.buf[6+ln+j]);  /** Data */

	crc = crc16modbus(b->buf, 6+ln+sch);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

