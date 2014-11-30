
#include "uso_threads.h"
#include "uso.h"
#include "ocs.h"
#include "bologging.h"
#include "serial.h"


int apsv[256] = {0};
int apsv_ln = 0;

unsigned int p11, p12, p13;
unsigned int p21, p22, p23;


int tx_uso(struct actx_thread_arg *targ)
{
	char buf[BUF485_SZ];  /** Буфер передатчика RS485 */
	int res;

	/** Tx */
	res = writer(&txBuf, buf, targ->port);	
	if (res < 0) return -1;
	
	return 0;
}

int rx_uso2(struct actx_thread_arg *targ)
{
	char buf[BUF485_SZ];  /** Буфер приемника RS485 */
	int res;

	/** Rx */	
	put_rxFl(&rxBuf, RX_WAIT);
	rxBuf.wpos = 0;
	res = 1;
	
	while (res) {
		res = reader(&rxBuf, buf, targ->port, targ->tout);
		if (res < 0) return -1;
	}
	
	return 0;
}


int rx_uso(struct actx_thread_arg *targ)
{
	char buf[BUF485_SZ];  /** Буфер приемника RS485 */
	int i;
	int n;        /** Кол-во байт принятых функцией SerialBlockRead() */
	
	write(1, "rx:\n", 4);

	put_rxFl(&rxBuf, RX_WAIT);
	rxBuf.wpos = 0;

	while (1) {
		n = SerialBlockRead(targ->port, buf, BUF485_SZ);

		if (n < 0) {
			bo_log("rx_485: SerialBlockRead exit");
			return -1;
		}
		
		for (i=0; i<n; i++) {
			/* bo_log("rx_485: buf[i]= %d", (unsigned int)buf[i]);
			printf("0x%02x\n", (unsigned char)buf[i]); */
			
			put_rxFl(&rxBuf,
				 read_byte(&rxBuf, buf[i], get_rxFl(&rxBuf)));
			if (get_rxFl(&rxBuf) >= RX_DATA_READY) break;		
		}

		if (get_rxFl(&rxBuf) >= RX_DATA_READY) {
			/** Данные приняты */
			break;
		}
	}  /** while */

	return 0;
}

int uso1_proc(struct actx_thread_arg *targ, int ncmd)
{
	int i;

	if (ncmd == 1) {
		for (i=0; i<apsv_ln; i++)
			if (apsv[i] == targ->pr1) {
				p11 = uso(targ, &txBuf, p11, targ->pr1);
				break;
			}
	} else if (ncmd == 2) {
		for (i=0; i<apsv_ln; i++)
			if (apsv[i] == targ->pr2) {
				p12 = uso(targ, &txBuf, p12, targ->pr2);
				break;
			}
	} else if (ncmd == 3) {
		for (i=0; i<apsv_ln; i++)
			if (apsv[i] == targ->pr3) {
				p13 = uso(targ, &txBuf, p13, targ->pr3);
				break;
			}
	} else {
		/**
		   uso_answer(targ, &txBuf); */
	}
	
	if (ncmd < apsv_ln)
		ncmd++;
	else
		ncmd = 1;

	return ncmd;
}

int uso2_proc(struct actx_thread_arg *targ, int ncmd)
{
	int i;

	if (ncmd == 1) {
		for (i=0; i<apsv_ln; i++)
			if (apsv[i] == targ->pr1) {
				p21 = uso(targ, &txBuf, p21, targ->pr1);
				break;
			}
	} else if (ncmd == 2) {
		for (i=0; i<apsv_ln; i++)
			if (apsv[i] == targ->pr2) {
				p22 = uso(targ, &txBuf, p22, targ->pr2);
				break;
			}
	} else if (ncmd == 3) {
		for (i=0; i<apsv_ln; i++)
			if (apsv[i] == targ->pr3) {
				p23 = uso(targ, &txBuf, p23, targ->pr3);
				break;
			}
	} else {
		/**
		   uso_answer(targ, &txBuf); */
	}
	
	if (ncmd < apsv_ln)
		ncmd++;
	else
		ncmd = 1;

	return ncmd;
}

void uso_printLog()
{
	/* int nn; */
	int l, n;
	int i;
	unsigned int ptm;
	unsigned int dst, src, req, dev, stat, ln, dsz;
	char dt[6] = {0};
	char data[1024];
	
	/* nn = rxBuf.buf[5]; */
	l = (rxBuf.buf[6] & 0xFF) | (rxBuf.buf[7] << 8);
	n = (rxBuf.buf[8] & 0xFF) | (rxBuf.buf[9] << 8);
	ln = 0;
	
	while (n--) {
		ptm = ((unsigned char)rxBuf.buf[10+ln] |
		       (unsigned char)rxBuf.buf[11+ln] << 8 |
		       (unsigned char)rxBuf.buf[12+ln] << 16 |
		       (unsigned char)rxBuf.buf[13+ln] << 24) & 0xffffffff;
		dst = (unsigned char)rxBuf.buf[14+ln];
		src = (unsigned char)rxBuf.buf[15+ln];
		req = (unsigned char)rxBuf.buf[16+ln];
		dev = (unsigned char)rxBuf.buf[17+ln];
		stat = (unsigned char)rxBuf.buf[18+ln];
		dsz = (unsigned char)rxBuf.buf[19+ln];

		memset(data, 0, 1024);
		sprintf(dt, " 0x%02x", dst);
		strncat(data, dt, 5);
		sprintf(dt, " 0x%02x", src);
		strncat(data, dt, 5);
		sprintf(dt, " 0x%02x", req);
		strncat(data, dt, 5);
		sprintf(dt, " 0x%02x", dev);
		strncat(data, dt, 5);
		sprintf(dt, " 0x%02x", stat);
		strncat(data, dt, 5);
		sprintf(dt, " 0x%02x", dsz);
		strncat(data, dt, 5);
		
		strncat(data, ":", 1);
		
		for (i=0; i<dsz; i++) {
			sprintf(dt, " 0x%02x", rxBuf.buf[20+ln+i]);
			strncat(data, dt, 5);
		}
		
		/* printf("%u[%u] %s\n", l, ptm, data); */
		bo_log("%d[%d] %s", l, ptm, data);

		l++;
		ln += dsz + 10;
	}
}

int uso_session(struct actx_thread_arg *targ, const char *msg,
		int ncmd, int adr, int logger)
{
	int i;

	if (rxBuf.buf[2] == (char)targ->cdaId) {
		if (logger)
			uso_quLog(targ, &txBuf, targ->lline, targ->nllines);
		else
			if (apsv_ln > 0) {
				if (adr == targ->adr1)
					ncmd = uso1_proc(targ, ncmd);
				else if (adr == targ->adr2)
					ncmd = uso2_proc(targ, ncmd);
				else
					bo_log("unknown adr= %d", adr);
			} else {
				bo_log("%s answer apsv= {}", msg);
				uso_answer(targ, &txBuf);
			}
	} else if (rxBuf.buf[2] == (char)targ->cdnsId) {
		/* write(1, "uso: netstatus()\n", 17); */
		apsv_ln = rxBuf.buf[5];
		for (i=0; i<apsv_ln; i++)
			apsv[i] = rxBuf.buf[6+i];
		/* bo_log("uso: netstatus answer"); */
		uso_answer(targ, &txBuf);
	} else if (rxBuf.buf[2] == (char)targ->cdquLogId) {
		/** Print Log
		uso_printLog(); */
		uso_answer(targ, &txBuf);
	} else {
		bo_log("%s ??? id= %d", msg, (unsigned int)rxBuf.buf[2]);
		uso_answer_error(targ, &txBuf);
	}

	return ncmd;
}

void *actx_485(void *arg)
{
	struct actx_thread_arg *targ = (struct actx_thread_arg *)arg;
	int ncmd1 = 1;
	int ncmd2 = 1;
	int res;

	p11 = 1;
	p12 = 1;
	p13 = 1;
	p21 = 1;
	p22 = 1;
	p23 = 1;
	
	write(1, "uso:\n", 5);

	while (1) {
		res = rx_uso(targ);
		if (res < 0) {
			bo_log("actx_485: rx_uso exit");
			break;
		}
		
		if ((rxBuf.buf[0] == (char)targ->adr1) ||
		    (rxBuf.buf[0] == (char)targ->adr2)) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				/** USO */
				if (rxBuf.buf[0] == (char)targ->adr1) {
					/** USO1 */
					if (rxBuf.wpos == 4) {
						/** Нас сканируют */
						write(1, "uso1: scan()\n", 13);
						scan(targ, &txBuf);
					
					} else {
						write(1, "uso1: active()\n", 15);
						ncmd1 = uso_session(targ, "USO1:",
								    ncmd1,
								    targ->adr1,
								    targ->logger1);
					}
				
				} else if (rxBuf.buf[0] == (char)targ->adr2) {
					/** USO1 */
					if (rxBuf.wpos == 4) {
						/** Нас сканируют */
						write(1, "uso2: scan()\n", 13);
						scan(targ, &txBuf);
					
					} else {
						write(1, "uso2: active()\n", 15);
						ncmd2 = uso_session(targ, "USO2:",
								    ncmd2,
								    targ->adr2,
								    targ->logger2);
					}
				} else
					break;
				
				res = tx_uso(targ);
				if (res < 0) {
					bo_log("actx_485: tx_uso exit");
					pthread_exit(0);
				}
				break;
			case RX_ERROR:
				/** Ошибка кадра */
				bo_log("actx_485(): Cadr Error !");
				break;
			case RX_TIMEOUT:
				/** Текущее устройство не отвечает */
				bo_log("actx_485(): timeout dst= %d",
				       (unsigned int)rxBuf.buf[1]);
				break;
			default:
				bo_log("actx_485(): state ??? fl= %d",
				       get_rxFl(&rxBuf));
				break;
			}
		}
	}
	
	bo_log("actx_485: exit");
	pthread_exit(0);
}

