
#include "uso.h"
#include "ocrc.h"
#include "bologging.h"


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

	cfg_put(cfg, "USO1:adr", "229");
	cfg_put(cfg, "USO1:logger", "0");

	cfg_put(cfg, "USO2:adr", "230");
	cfg_put(cfg, "USO2:logger", "1");

	cfg_put(cfg, "LOGGER:line", "1");
	cfg_put(cfg, "LOGGER:nlines", "4");

	/** Количество данных в кадре теста */
	cfg_put(cfg, "TEST:lnTest", "1");
	
	/** Идентификаторы подсистем ЛС */
	cfg_put(cfg, "LS:gen", "General");         /** 0xC2*/

	/** Запросы */
	cfg_put(cfg, "REQ:ag", "AccessGranted");     /** 0xE6 */
	cfg_put(cfg, "REQ:ns", "GetNetworkStatus");  /** 0x29 */
	cfg_put(cfg, "REQ:gl", "GetLog");            /** 0x0E */

	cfg_put(cfg, "PR:adr1", "2");
	cfg_put(cfg, "PR:adr2", "3");
	cfg_put(cfg, "PR:adr3", "4");
	cfg_put(cfg, "PR:adr4", "5");

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

unsigned int uso(struct actx_thread_arg *targ, struct thr_tx_buf *b,
		 unsigned int sch, int dst)
{
	unsigned int crc;
	int i;
	char csch[10];
	int ln = 8;
	
	sprintf(csch, "%08u", sch);
	
	b->wpos = 0;
	put_txBuf(b, (char)dst);     /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */

	put_txBuf(b, (char)targ->cdaId);
	put_txBuf(b, (char)targ->cdaDest);
	put_txBuf(b, (char)1);
	put_txBuf(b, (char)ln);
	
	for (i=0; i<ln; i++)
		put_txBuf(b, csch[i]);

	for (i=0; i<targ->test_ln; i++)
		put_txBuf(b, '\x00');
	
	crc = crc16modbus(b->buf, ln+6+targ->test_ln);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));

	bo_log("uso(): sch=%d dst=%d src=%d",
	       sch,
	       (unsigned char)dst,
	       (unsigned char)rxBuf.buf[0]);

	sch++;
	
	return sch;
}

void uso_quNetStat(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;

	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);  /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */

	put_txBuf(b, (char)targ->cdnsId);
	put_txBuf(b, (char)targ->cdnsDest);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)0);

	crc = crc16modbus(b->buf, 7);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

void uso_quLog(struct actx_thread_arg *targ, struct thr_tx_buf *b,
	       unsigned int line, unsigned int nlines)
{
	unsigned int crc;

	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);
	put_txBuf(b, rxBuf.buf[0]);
	put_txBuf(b, (char)targ->cdquLogId);
	put_txBuf(b, (char)targ->cdquDest);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)4);
	put_txBuf(b, (char)(line & 0xff));
	put_txBuf(b, (char)((line >> 8) & 0xff));
	put_txBuf(b, (char)(nlines & 0xff));
	put_txBuf(b, (char)((nlines >> 8) & 0xff));

	crc = crc16modbus(b->buf, 10);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

void uso_answer(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;

	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);  /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */

	put_txBuf(b, rxBuf.buf[2]);
	put_txBuf(b, rxBuf.buf[3]);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)0);
	
	crc = crc16modbus(b->buf, 6);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

void uso_answer_error(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;

	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);  /** dst */
	put_txBuf(b, rxBuf.buf[0]);  /** src */

	put_txBuf(b, rxBuf.buf[2]);
	put_txBuf(b, rxBuf.buf[3]);
	put_txBuf(b, (char)7);
	put_txBuf(b, (char)0);
	
	crc = crc16modbus(b->buf, 6);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));
}

