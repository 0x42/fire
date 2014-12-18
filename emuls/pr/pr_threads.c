
#include "pr.h"
#include "ocs.h"
#include "bologging.h"
#include "serial.h"


unsigned int p1, p2;


int tx_pr(struct actx_thread_arg *targ)
{
	char buf[BUF485_SZ];  /** Буфер передатчика RS485 */
	int res;

	/** Tx */
	res = writer(&txBuf, buf, targ->port);
	if (res < 0) return -1;
	
	return 0;
}

int rx_pr2(struct actx_thread_arg *targ)
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

int rx_pr(struct actx_thread_arg *targ)
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

int pr_proc(int adr, unsigned int sch, int ln, int msgln, char *msg)
{
	char csch[10];
	unsigned int lngth;
	unsigned int pn;
	int i;
	int res = 0;

	memset(csch, 0, 10);

	/*
	0x03 0x9f 0xe6 0xc2 0x01 0x13:uso2(159)-00000833 0x01
	*/
	
	lngth = (unsigned int)rxBuf.buf[5];
	for (i=0; i<lngth-ln-msgln; i++)
		csch[i] = (unsigned char)rxBuf.buf[msgln+6+i];
	
	pn = (unsigned int)atoi(csch);

	
	printf("pr_proc(): sch=%d dst=%d src=%d [%02x %02x %02x %02x] %c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c%c\n]",
	       sch,
	       (unsigned char)rxBuf.buf[0],
	       (unsigned char)rxBuf.buf[1],
	       (unsigned char)rxBuf.buf[2],
	       (unsigned char)rxBuf.buf[3],
	       (unsigned char)rxBuf.buf[4],
	       (unsigned char)rxBuf.buf[5],
	       
	       (unsigned char)rxBuf.buf[6],
	       (unsigned char)rxBuf.buf[7],
	       (unsigned char)rxBuf.buf[8],
	       (unsigned char)rxBuf.buf[9],
	       (unsigned char)rxBuf.buf[10],
	       (unsigned char)rxBuf.buf[11],
	       (unsigned char)rxBuf.buf[12],
	       (unsigned char)rxBuf.buf[13],
	       (unsigned char)rxBuf.buf[14],
	       (unsigned char)rxBuf.buf[15],
	       (unsigned char)rxBuf.buf[16],
	       (unsigned char)rxBuf.buf[17],
	       (unsigned char)rxBuf.buf[18],
	       (unsigned char)rxBuf.buf[19],
	       (unsigned char)rxBuf.buf[20],
	       (unsigned char)rxBuf.buf[21],
	       (unsigned char)rxBuf.buf[22],
	       (unsigned char)rxBuf.buf[23]
		);
	
	
	if ((sch == 1) && (pn != sch)) {
		printf("Sync packets %s\n", msg);
		res = pn;
	} else if (pn != sch) {
		bo_log("Loss packet USO(%d)= %d/ pn= %d", adr, sch, pn);
		printf("Loss packet %s %d\n", msg, sch);
		/* sch = pn; */
		res = -1;
	} else if (pn == sch-1) {
		bo_log("Duplicate packet USO(%d)= %d", adr, sch);
		printf("Duplicate packet %s %d\n", msg, sch);
		/* sch = pn; */
		res = -1;
	} else if (pn == sch) {
		/* bo_log("USO(%d) ______ ok", adr);
		printf("%s %d ______ ok\n", msg, sch); */
		res = 0;
	} else {
		bo_log("??? USO(%d)= %d/ pn=%d", adr, sch, pn);
		printf("??? %s %d\n", msg, sch);
		res = -1;
	}
	
	return res;
}


void *actx_485(void *arg)
{
	struct actx_thread_arg *targ = (struct actx_thread_arg *)arg;
	int res;
	int p;
	
	p1 = 1;
	p2 = 1;

	write(1, "pr:\n", 4);

	while (1) {
		res = rx_pr(targ);
		if (res < 0) {
			bo_log("actx_485: rx_pr exit");
			break;
		}
		
		if (rxBuf.buf[0] == (char)targ->adr) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				/** PR */
				if (rxBuf.wpos == 4) {
					/** Нас сканируют */
					write(1, "pr: scan()\n", 11);
					scan(targ, &txBuf);
				} else {
					write(1, "pr: action()\n", 13);
					if ((unsigned char)rxBuf.buf[1] == targ->uso1) {
						p = pr_proc(targ->uso1,
							    p1,
							    targ->test1_ln,
							    targ->test1_msgln,
							    targ->test1_msg);
						if (p == -1) {
							p1 = 1;
							printf("%s TEST ERROR\n", targ->test1_msg);
							bo_log("actx_485: test error exit");
							/** pthread_exit(0); */
						}
						if (p > 0) p1 = p;
						if (p1 < targ->test1_m)
							p1++;
						else {
							p1 = 1;
							printf("%s TEST OK\n", targ->test1_msg);
							bo_log("actx_485: test ok exit");
							/** pthread_exit(0); */
						}

					} else if ((unsigned char)rxBuf.buf[1] == targ->uso2) {
						p = pr_proc(targ->uso2,
							    p2,
							    targ->test2_ln,
							    targ->test2_msgln,
							    targ->test2_msg);
						if (p == -1) {
							p2 = 1;
							printf("%s TEST ERROR\n", targ->test2_msg);
							bo_log("actx_485: test error exit");
							/** pthread_exit(0); */
						}
						if (p > 0) p2 = p;
						if (p2 < targ->test1_m)
							p2++;
						else {
							p2 = 1;
							printf("%s TEST OK\n", targ->test2_msg);
							bo_log("actx_485: test ok exit");
							/** pthread_exit(0); */
						}
					} else {
						bo_log("actx_485: break");
						break;
					}
					
					probot(targ, &txBuf);
				}
				
				res = tx_pr(targ);
				if (res < 0) {
					bo_log("actx_485: tx_pr exit");
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
				bo_log("actx_485(): state unknown fl= %d",
				       get_rxFl(&rxBuf));
				break;
			}
		}
	}

	bo_log("actx_485: exit");
	pthread_exit(0);
}

