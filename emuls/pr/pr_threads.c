
#include "pr.h"
#include "bologging.h"


void *actx_485(void *arg)
{
	struct actx_thread_arg *targ = (struct actx_thread_arg *)arg;
	int res;
	
	write(1, "pr:\n", 4);

	while (1) {
		res = rx(targ->port);
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
					scan(&txBuf);
				} else {
					/**
					   write(1, "pr: from active\n", 16);
					*/
					probot(&txBuf, targ->test_msgln);
				}
				
				res = tx(targ->port);
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

