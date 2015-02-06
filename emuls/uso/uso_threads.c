
#include "uso.h"
#include "bologging.h"


int apsv[256] = {0};
int apsv_ln = 0;

int nm;
int rcv_nm;
int rcv_sch;
int rcv_ok = 0;


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
			if ((rxBuf.buf[20+ln+i] > 32) &&
			    (rxBuf.buf[20+ln+i] < 128)) {
				sprintf(dt, "%c", rxBuf.buf[20+ln+i]);
				strncat(data, dt, 1);
			} else {
				sprintf(dt, " 0x%02x", rxBuf.buf[20+ln+i]);
				strncat(data, dt, 5);
			}
		}
		
		/* printf("%u[%u] %s\n", l, ptm, data); */
		bo_log("%d[%d] %s", l, ptm, data);

		l++;
		ln += dsz + 10;
	}
}

void uso_print_ms() 
{
	char data[1024];
	int j;
	unsigned int ln;
	
	memset(data, 0, 1024);
	ln = (rxBuf.buf[10] & 0xff) | (rxBuf.buf[11] << 8);

	for (j=0; j<ln; j++) {
		data[j] = (unsigned char)rxBuf.buf[12+j];
	}

	bo_log("uso_print_ms");
	bo_log("data [%s]", data);
	
	printf("dst=[%d] <- src=[%d] [%s]\n",
	       (unsigned char)rxBuf.buf[0],
	       (unsigned char)rxBuf.buf[1],
	       data);
}

int uso_tx(struct actx_thread_arg *targ, unsigned int sch)
{
	int res;
	char tmstr[50] = {0};
	
	/**
	int i, j;
	char data[1200];
	unsigned char ln;
	*/
	
	res = uso(targ, &txBuf, sch, targ->pr);
	if (res == -1) return -1;

	bo_getTimeNow(tmstr, 50);
	
	printf("USO[%s]: to   passive [%d]      ------>>-->>-----\n", tmstr, sch);
	
	/**
	memset(data, 0, 1200);
	ln = (unsigned char)txBuf.buf[5];

	for (i=0; i<ln; i++) {
		data[i] = (unsigned char)txBuf.buf[6+i];
	}

	for (j=0; j<sch; j++) {
		data[ln+j] = (unsigned char)txBuf.buf[6+ln+j];
	}

	printf("sch=%d dst=[%d] <- src=[%d] [%02x %02x %02x %02x] [%s]\n",
	       sch,
	       (unsigned char)txBuf.buf[0],
	       (unsigned char)txBuf.buf[1],
	       (unsigned char)txBuf.buf[2],
	       (unsigned char)txBuf.buf[3],
	       (unsigned char)txBuf.buf[4],
	       (unsigned char)txBuf.buf[5],
	       data);
	*/
	
	return 0;
}

int uso_print_sq_new(struct actx_thread_arg *targ, unsigned int sch) 
{
	int j;
	char data[1200] = {0};
	int res = 0;

	for (j=0; j<rxBuf.wpos; j++) {
		data[j] = (unsigned char)rxBuf.buf[j];
	}
	
	printf("[%s]\n", data);
	
	return res;
}

void uso_print_sq(struct actx_thread_arg *targ, unsigned int sch) 
{
	char csch[10];
	unsigned char ln;
	unsigned int rxsch;
	int i, j;
	char data[1200];
	char tmstr[50] = {0};

	memset(csch, 0, 10);

	ln = (unsigned char)rxBuf.buf[5];
	
	for (i=0; i<ln-targ->test_msgln; i++)
		csch[i] = (unsigned char)rxBuf.buf[6+targ->test_msgln+i];
	
	rxsch = (unsigned int)atoi(csch);
	
	memset(data, 0, 1200);

	for (j=0; j<ln; j++) {
		data[j] = (unsigned char)rxBuf.buf[6+j];
	}

	for (j=0; j<rxsch; j++) {
		data[ln+j] = (unsigned char)rxBuf.buf[6+ln+j];
	}

	bo_getTimeNow(tmstr, 50);

	printf("USO[%s]: from passive      [%d] ------<<--<<-----\n", tmstr, rxsch);
	
	/**
	printf("sch=%d dst=[%d] <- src=[%d] [%02x %02x %02x %02x] [%s]\n",
	       rxsch,
	       (unsigned char)rxBuf.buf[0],
	       (unsigned char)rxBuf.buf[1],
	       (unsigned char)rxBuf.buf[2],
	       (unsigned char)rxBuf.buf[3],
	       (unsigned char)rxBuf.buf[4],
	       (unsigned char)rxBuf.buf[5],
	       data);
	*/
	
	/** 
	if (rxsch != sch) {
		bo_log("%s TEST ERROR: Loss packet sch= %d/ recv_sch= %d",
		       targ->test_msg, sch, rxsch);
		printf("%s TEST ERROR: Loss packet sch= %d/ recv_sch= %d\n",
		       targ->test_msg, sch, rxsch);
		res = rxsch;
	}

	rcv_ok = 1; */
}

int uso_proc(struct actx_thread_arg *targ, unsigned int sch)
{
	int res;
	int i;
	int fl = 1;

	for (i=0; i<apsv_ln; i++)
		if (apsv[i] == targ->pr) {
			fl = 0;
			/* rcv_ok = 0; */
			if (targ->test_nm > 0) {
				/** Выполнять NM блоков размером в N
				 * запросов */
				if (nm <= targ->test_nm) {
					res = uso_tx(targ, sch);
					if (res == -1) break;
					
					if (sch < (targ->test_m + targ->test_ln))
						sch++;
					else {
						sch = (unsigned int)targ->test_ln;
						nm++;
					}
					
					res = sch;
				} else {
					/** Стоп
					    rcv_ok = 1;
					
					    bo_log("actx_485: uso_answer()"); */
					res = uso_answer(targ, &txBuf);
					if (res < 0)
						res = -1;
					else
						res = -2;
				}
			} else {
				/** Выполнять блок размером в N запросов
				 * бесконечно долго */
				res = uso_tx(targ, sch);
				if (res == -1) break;
				
				if (sch < (targ->test_m + targ->test_ln))
					sch++;
				else
					sch = (unsigned int)targ->test_ln;
				
				res = sch;
			}
			
			break;
		}
	if (fl)
		res = uso_quNetStat(targ, &txBuf);

	return res;
}

int uso_session(struct actx_thread_arg *targ, unsigned int sch)
{
	int i, j;
	int n;
	int tst;
	int res = 0;

	if (rxBuf.buf[2] == (char)targ->cdaId) {
		if (targ->logger) {
			bo_log("actx_485: uso_quLog()");
			res = uso_quLog(targ, &txBuf, targ->lline, targ->nllines);
		} else if (targ->snmp_q) {
			bo_log("actx_485: uso_quSNMP()");
			res = uso_quSNMP(targ, &txBuf);
		} else {
			if (apsv_ln > 0) {
				if (rcv_ok == 0) {
					/** Послать запрос устройству
					    printf("actx_485: next zapros\n"); */
					res = uso_proc(targ, sch);
				} else {
					/**
					bo_log("actx_485:
					uso_answer()"); */
					res = uso_answer(targ, &txBuf);
				}
			} else {
				bo_log("actx_485: uso_quNetStat()");
				res = uso_quNetStat(targ, &txBuf);
			}
		}
	} else if (rxBuf.buf[2] == (char)targ->cdsqId) {
		/** Ответ от устройства
		bo_log("actx_485: from PR"); */
		uso_print_sq(targ, sch);
		res = uso_answer(targ, &txBuf);
	} else if (rxBuf.buf[2] == (char)targ->cdmsId) {
		printf("USO: magistral status\n");
		uso_print_ms();
		res = uso_answer(targ, &txBuf);
	} else if (rxBuf.buf[2] == (char)targ->cdnsId) {
		printf("uso_session: cdns\n");
		/**
		 *                 0         1            31
		 *                 |         |            |
		 * adr: -> bitAdr: 1000 0000 1000 0000 .. 0000 0000
		 *                 |       | |       |    |       |
		 *                 8       1 15      9    256     249
		 */
		n = rxBuf.buf[5];
		apsv_ln = 0;
		memset(apsv, 0, 256);
		
		for (i=0; i<n; i++)
			for (j=0; j<8; j++) {
				tst = (rxBuf.buf[6+i] & (1<<j));
				if (tst > 0) {
					bo_log("uso: (adr=%d)", i*8+j+1);
					apsv[apsv_ln] = i*8+j+1;
					apsv_ln++;
				}
			}
		res = uso_answer(targ, &txBuf);
	} else if (rxBuf.buf[2] == (char)targ->cdquLogId) {
		/** Print Log */
		if (targ->logger) uso_printLog();
		res = uso_answer(targ, &txBuf);
	} else {
		bo_log("USO: ??? id= %d", (unsigned int)rxBuf.buf[2]);
		printf("uso_session: error\n");
	}

	return res;
}

void *actx_485(void *arg)
{
	struct actx_thread_arg *targ = (struct actx_thread_arg *)arg;
	int res;
	unsigned int sch;
	char tmstr[50] = {0};
	
	sch = (unsigned int)targ->test_ln;
	nm = 1;

	rcv_sch = (unsigned int)targ->test_ln;
	rcv_nm = 1;

	rcv_ok = 0;
	
	write(1, "uso:\n", 5);

	while (1) {
		/**
		memset(tmstr, 0, 50);
		
		bo_getTimeNow(tmstr, 50);

		printf("actx_485[%s]\n", tmstr);
		*/
		res = rx(targ);
		if (res < 0) {
			bo_log("actx_485: rx() exit");
			break;
		}
		
		if (rxBuf.buf[0] == (char)targ->adr) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				/** USO */
				if (rxBuf.wpos == 4) {
					/** Нас сканируют */
					write(1, "uso: scan()\n", 13);
					res = scan(targ, &txBuf);
					if (res < 0) {
						bo_log("actx_485: scan() exit");
						pthread_exit(0);
					}
				} else {
					/**
					write(1, "uso: active()\n", 15);
					*/
					res = uso_session(targ, sch);
					if (res == -1)
						pthread_exit(0);
					else if (res == -2) {
						/** Стоп */
						printf("actx_485: USO TEST STOP\n");
						rcv_ok = 1;
						/* pthread_exit(0); */
					} else if (res > 0)
						sch = res;
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

