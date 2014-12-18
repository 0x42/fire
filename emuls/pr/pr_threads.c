
#include "pr.h"
#include "ocs.h"
#include "bologging.h"
#include "serial.h"


unsigned int p11, p12, p13, p14;
unsigned int p21, p22, p23, p24;


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

unsigned int pr_proc(int adr, unsigned int sch)
{
	char csch[10];
	unsigned int ln;
	unsigned int pn;
	int i;

	printf("USO(%d): %u\n", adr, sch);

	memset(csch, 0, 10);

	ln = (unsigned int)rxBuf.buf[5];
	for (i=0; i<ln; i++)
		csch[i] = (unsigned char)rxBuf.buf[6+i];
	
	pn = (unsigned int)atoi(csch);

	/*
	bo_log("pr_proc(): sch=%d dst=%d src=%d",
	       sch,
	       (unsigned char)rxBuf.buf[1],
	       (unsigned char)rxBuf.buf[0]);
	*/
	
	if (sch == 1)
		sch = pn;
	else if (pn != sch) {
		bo_log("Loss packet USO(%d)= %d/ pn= %d", adr, sch, pn);
		sch = pn;
	} else if (pn == sch-1) {
		bo_log("Duplicate packet USO(%d)= %d", adr, sch);
		sch = pn;
	} else if (pn == sch) {
		/* bo_log("USO(%d) ______ ok", adr); */
	} else
		bo_log("??? USO(%d)= %d/ pn=%d", adr, sch, pn);
	
	sch++;

	return sch;
}


void *actx_485(void *arg)
{
	struct actx_thread_arg *targ = (struct actx_thread_arg *)arg;
	int res;
	
	p11 = 1;
	p12 = 1;
	p13 = 1;
	p14 = 1;
	p21 = 1;
	p22 = 1;
	p23 = 1;
	p24 = 1;

	write(1, "pr:\n", 4);

	while (1) {
		res = rx_pr(targ);
		if (res < 0) {
			bo_log("actx_485: rx_pr exit");
			break;
		}
		
		if ((rxBuf.buf[0] == (char)targ->adr1) ||
		    (rxBuf.buf[0] == (char)targ->adr2)) {
			switch (get_rxFl(&rxBuf)) {
			case RX_DATA_READY:
				/** PR */
				if (rxBuf.buf[0] == (char)targ->adr1) {
					/** PR1 */
					if (rxBuf.wpos == 4) {
						/** Нас сканируют */
						write(1, "pr1: scan()\n", 12);
						scan(targ, &txBuf);
					} else {
						if ((unsigned char)rxBuf.buf[1] == targ->uso1)
							p11 = pr_proc(targ->uso1, p11);
						else if ((unsigned char)rxBuf.buf[1] == targ->uso2)
							p12 = pr_proc(targ->uso2, p12);
						else if ((unsigned char)rxBuf.buf[1] == targ->uso3)
							p13 = pr_proc(targ->uso3, p13);
						else if ((unsigned char)rxBuf.buf[1] == targ->uso4)
							p14 = pr_proc(targ->uso4, p14);
						else
							break;

						probot(targ, &txBuf);
					}
					
				} else if (rxBuf.buf[0] == (char)targ->adr2) {
					/** PR2 */
					if (rxBuf.wpos == 4) {
						/** Нас сканируют */
						write(1, "pr2: scan()\n", 12);
						scan(targ, &txBuf);
					} else {
						if ((unsigned char)rxBuf.buf[1] == targ->uso1)
							p21 = pr_proc(targ->uso1, p21);
						else if ((unsigned char)rxBuf.buf[1] == targ->uso2)
							p22 = pr_proc(targ->uso2, p22);
						else if ((unsigned char)rxBuf.buf[1] == targ->uso3)
							p23 = pr_proc(targ->uso3, p23);
						else if ((unsigned char)rxBuf.buf[1] == targ->uso4)
							p24 = pr_proc(targ->uso4, p24);
						else
							break;
						
						probot(targ, &txBuf);
					}
				} else
					break;
				
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
				bo_log("actx_485(): timeout dst= %d", (unsigned int)rxBuf.buf[1]);
				break;
			default:
				bo_log("actx_485(): state unknown fl= %d", get_rxFl(&rxBuf));
				break;
			}
		}
	}

	bo_log("actx_485: exit");
	pthread_exit(0);
}

