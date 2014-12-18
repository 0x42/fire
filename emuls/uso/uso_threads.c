
#include "uso.h"
#include "ocs.h"
#include "bologging.h"
#include "serial.h"


int apsv[256] = {0};
int apsv_ln = 0;
unsigned int p1, p2;

int spr = 0;


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

void uso_proc(struct actx_thread_arg *targ)
{
	int i;
	int fl = 1;

	if (spr) {
		spr = 0;
		for (i=0; i<apsv_ln; i++)
			if (apsv[i] == targ->pr1) {
				p1 = uso(targ, &txBuf, p1, targ->pr1);
				fl = 0;
				printf("pr_proc(): sch=%d dst=%d src=%d [%02x %02x %02x %02x] %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n]",
				       p1,
				       (unsigned char)txBuf.buf[0],
				       (unsigned char)txBuf.buf[1],
				       (unsigned char)txBuf.buf[2],
				       (unsigned char)txBuf.buf[3],
				       (unsigned char)txBuf.buf[4],
				       (unsigned char)txBuf.buf[5],
	       
				       (unsigned char)txBuf.buf[6],
				       (unsigned char)txBuf.buf[7],
				       (unsigned char)txBuf.buf[8],
				       (unsigned char)txBuf.buf[9],
				       (unsigned char)txBuf.buf[10],
				       (unsigned char)txBuf.buf[11],
				       (unsigned char)txBuf.buf[12],
				       (unsigned char)txBuf.buf[13],
				       (unsigned char)txBuf.buf[14],
				       (unsigned char)txBuf.buf[15],
				       (unsigned char)txBuf.buf[16],
				       (unsigned char)txBuf.buf[17],
				       (unsigned char)txBuf.buf[18],
				       (unsigned char)txBuf.buf[19],
				       (unsigned char)txBuf.buf[20],
				       (unsigned char)txBuf.buf[21],
				       (unsigned char)txBuf.buf[22],
				       (unsigned char)txBuf.buf[23]
					);
				break;
			}
	} else {
		spr = 1;
		for (i=0; i<apsv_ln; i++)
			if (apsv[i] == targ->pr2) {
				p2 = uso(targ, &txBuf, p2, targ->pr2);
				fl = 0;
				printf("pr_proc(): sch=%d dst=%d src=%d [%02x %02x %02x %02x] %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n]",
				       p1,
				       (unsigned char)txBuf.buf[0],
				       (unsigned char)txBuf.buf[1],
				       (unsigned char)txBuf.buf[2],
				       (unsigned char)txBuf.buf[3],
				       (unsigned char)txBuf.buf[4],
				       (unsigned char)txBuf.buf[5],
	       
				       (unsigned char)txBuf.buf[6],
				       (unsigned char)txBuf.buf[7],
				       (unsigned char)txBuf.buf[8],
				       (unsigned char)txBuf.buf[9],
				       (unsigned char)txBuf.buf[10],
				       (unsigned char)txBuf.buf[11],
				       (unsigned char)txBuf.buf[12],
				       (unsigned char)txBuf.buf[13],
				       (unsigned char)txBuf.buf[14],
				       (unsigned char)txBuf.buf[15],
				       (unsigned char)txBuf.buf[16],
				       (unsigned char)txBuf.buf[17],
				       (unsigned char)txBuf.buf[18],
				       (unsigned char)txBuf.buf[19],
				       (unsigned char)txBuf.buf[20],
				       (unsigned char)txBuf.buf[21],
				       (unsigned char)txBuf.buf[22],
				       (unsigned char)txBuf.buf[23]
					);
				break;
			}
	}
	
	if (fl) uso_quNetStat(targ, &txBuf);
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
	char dt[2] = {0};
	
	int j;
	unsigned int ln;
	
	memset(data, 0, 1024);
	ln = (rxBuf.buf[10] & 0xff) | (rxBuf.buf[11] << 8);

	for (j=0; j<ln; j++) {
		/*
		sprintf(dt, "%c", (unsigned char)rxBuf.buf[12+j]);
		strncat(data, dt, 1);
		*/
		data[j] = (unsigned char)rxBuf.buf[12+j];
	}

	bo_log("uso_print_ms");
	bo_log("data [%s]", data);
	
	printf("dst=[%d] <- src=[%d] [%s]\n",
	       (unsigned char)rxBuf.buf[0],
	       (unsigned char)rxBuf.buf[1],
	       data);
}

int uso_session(struct actx_thread_arg *targ, int s)
{
	int i, j;
	int n;
	int tst;

	if (rxBuf.buf[2] == (char)targ->cdaId) {
		if (s) {
			s = 0;
			if (targ->logger)
				uso_quLog(targ, &txBuf, targ->lline, targ->nllines);
			else if (targ->snmp_q)
				uso_quSNMP(targ, &txBuf);
			else
				uso_answer(targ, &txBuf);
		} else {
			s = 1;
			if (apsv_ln > 0) {
				if (rxBuf.buf[4] == (char)1) {
					/** Ответ от устройства */ 
					printf("uso_session: operation complete\n");
					uso_answer(targ, &txBuf);
				} else {
					/** Послать запрос устройству */
					printf("uso_session: next zapros\n");
					uso_proc(targ);
				}
			} else {
				bo_log("USO: answer apsv= {}");
				uso_answer(targ, &txBuf);
			}
		}
		
	} else if (rxBuf.buf[2] == (char)targ->cdmsId) {
		write(1, "USO: magistral status()\n", 24);
		uso_print_ms();
		/* uso_answer(targ, &txBuf); */
	} else if (rxBuf.buf[2] == (char)targ->cdnsId) {
		/* write(1, "USO: netstatus()\n", 17); */
		/**
		 * adr: 15 -> bit adr: 1000 0000 0000 0000
		 *                     |                 |
		 *                    15                 0
		 */
		n = rxBuf.buf[5];
		apsv_ln = 0;
		memset(apsv, 0, 256);
		
		for (i=n-1; i>=0; i--)
			for (j=0; j<8; j++) {
				tst = (rxBuf.buf[6+i] & (1<<j));
				if (tst > 0) {
					/* bo_log("uso: netstatus answer (adr=%d)",
					   (n - 1 - i) * 8 + j); */
					apsv[apsv_ln] = (n - 1 - i) * 8 + j;
					apsv_ln++;
				}
			}
		
		/* bo_log("uso: netstatus answer (apsv_ln=%d)",
		   apsv_ln); */
		write(1, "uso_session: cdns\n", 18);
		uso_answer(targ, &txBuf);
	} else if (rxBuf.buf[2] == (char)targ->cdquLogId) {
		/** Print Log */
		if (targ->logger) uso_printLog();
		write(1, "uso_session: qulog\n", 19);
		uso_answer(targ, &txBuf);
	} else {
		bo_log("USO: ??? id= %d", (unsigned int)rxBuf.buf[2]);
		write(1, "uso_session: error\n", 19);
		uso_answer_error(targ, &txBuf);
	}

	return s;
}

void *actx_485(void *arg)
{
	struct actx_thread_arg *targ = (struct actx_thread_arg *)arg;
	int res;
	int s = 0;
	
	p1 = 1;
	p2 = 1;
	
	write(1, "uso:\n", 5);

	while (1) {
		res = rx_uso(targ);
		if (res < 0) {
			bo_log("actx_485: rx_uso exit");
			break;
		}
		
		if (rxBuf.buf[0] == (char)targ->adr) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				/** USO */
				if (rxBuf.wpos == 4) {
					/** Нас сканируют */
					write(1, "uso: scan()\n", 13);
					scan(targ, &txBuf);
					
				} else {
					write(1, "uso: active()\n", 15);
					s = uso_session(targ, s);
				}
				
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

