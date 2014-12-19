
#include "uso.h"
#include "ocrc.h"
#include "ort.h"
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

	cfg_put(cfg, "USO:adr", "229");
	cfg_put(cfg, "USO:logger", "0");
	cfg_put(cfg, "USO:snmp", "0");

	cfg_put(cfg, "LOGGER:line", "1");
	cfg_put(cfg, "LOGGER:nlines", "4");

	/** Идентификаторы подсистем ЛС */
	cfg_put(cfg, "LS:gen", "General");         /** 0xC2*/

	/** Запросы */
	cfg_put(cfg, "REQ:ag", "AccessGranted");     /** 0xE6 */
	cfg_put(cfg, "REQ:ms", "GetNetworkStatus");  /** 0x29 */
	cfg_put(cfg, "REQ:gl", "GetLog");            /** 0x0E */
	cfg_put(cfg, "REQ:ns", "GetNetRS485Status"); /** ???? */

	cfg_put(cfg, "REQ:sq", "StartQuench");       /** 0xC5 */

	cfg_put(cfg, "PR:adr1", "2");
	cfg_put(cfg, "PR:adr2", "3");

	/** Длина данных кадра */
	cfg_put(cfg, "TEST:ln", "10");
	/** Количество сообщений */
	cfg_put(cfg, "TEST:m", "100");
	/** Длина сообщения */
	cfg_put(cfg, "TEST:msg_ln", "10");
	/** Сообщение */
	cfg_put(cfg, "TEST:msg", "USO(229)- ");

	/** Тест SNMP */
	cfg_put(cfg, "SNMP:ip", "192.168.1.150");
	
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

	put_txBuf(b, (char)targ->cdsqId);
	put_txBuf(b, (char)targ->cdsqDest);
	put_txBuf(b, (char)0);
	put_txBuf(b, (char)ln);
	
	for (i=0; i<targ->test_msgln; i++)
		put_txBuf(b, targ->test_msg[i]);

	for (i=0; i<ln; i++)
		put_txBuf(b, csch[i]);

	for (i=0; i<targ->test_ln; i++)
		put_txBuf(b, '\x01');

	/** Установить длину сообщения */
	set_txBuf(b, 5, (char)(ln+targ->test_msgln+targ->test_ln));
	
	crc = crc16modbus(b->buf, ln+6+targ->test_msgln+targ->test_ln);

	put_txBuf(b, (char)(crc & 0xff));
	put_txBuf(b, (char)((crc >> 8) & 0xff));

	bo_log("uso(): sch=%d dst=%d src=%d",
	       sch,
	       (unsigned char)dst,
	       (unsigned char)rxBuf.buf[0]);

	if (sch < targ->test_m)
		sch++;
	else
		sch = 1;
	
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

void uso_quSNMP(struct actx_thread_arg *targ, struct thr_tx_buf *b)
{
	unsigned int crc;
	int buf[4];
	
	b->wpos = 0;
	put_txBuf(b, rxBuf.buf[1]);
	put_txBuf(b, rxBuf.buf[0]);
	put_txBuf(b, (char)targ->cdmsId);
	put_txBuf(b, (char)targ->cdmsDest);
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

