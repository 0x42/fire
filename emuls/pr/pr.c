
#include "pr.h"
#include "ocfg.h"
#include "ocrc.h"


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

	cfg_put(cfg, "PR:adr", "4");

	cfg_save(cfg, out);
	fclose(out);
	
	cfg_free(cfg);
}

void scan(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;
	
	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);  /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */
	
	crc = crc16modbus(b->buf, 2);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

void probot(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;
	int i;

	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);  /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */
	
	put_txBuf(b, rxBuf.buf[2]);  /** Id */
	put_txBuf(b, rxBuf.buf[3]);  /** Dest */
	put_txBuf(b, (char)0);       /** Status */
	put_txBuf(b, rxBuf.buf[5]);  /** DataSZ */

	for (i=0; i<rxBuf.buf[5]; i++)
		put_txBuf(b, rxBuf.buf[6+i]);  /** Data */

	crc = crc16modbus(b->buf, rxBuf.buf[5]+6);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

