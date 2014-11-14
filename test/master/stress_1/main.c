#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <pthread.h>
#include <time.h>

#include "../../../src/tools/ocrc.h"
#include "../../../src/tools/oht.h"
#include "../../../src/nettcp/bo_net_master_core.h"

char TR[100][23];
char TR_N[100][10];
int tr_n = 100;

struct thr_arg {
	int out1;
	int out2;
}; 

void recvNullTab(struct thr_arg *arg);

void init_TR()
{
	int i       = 0;
	int crc     = 0;
	char buf[2] = {0};
	char *str;
	char *str2;
	for(; i < tr_n; i++) {
		str = TR[i];
		str2 = TR_N[i];
		sprintf(str, "%03d:127.000.000.001:1", i);
		sprintf(str2, "%03d:NULL", i);
		
		crc = crc16modbus(str, 21);
		boIntToChar(crc, buf);
		str[21] = buf[0];
		str[22] = buf[1];
		
		crc = crc16modbus(str2, 8);
		boIntToChar(crc, buf);
		str2[8] = buf[0];
		str2[9] = buf[1];
	}
}

int sendTR(int sock)
{
	char *msg = NULL;
	int exec = -1;
	int i = 0;
	int ans = 1;
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
	for(;i < tr_n; i++) {
		msg = TR[i];
		exec = bo_sendSetMsg(sock, msg, 23);
		if(exec == -1) {
			printf("ERROR when SEND msg[");
			int j = 0;
			for(; j < 21; j++ ) printf("%c",TR[i][j]);
			printf("]\n");
			ans = -1;
			goto end;
		}
	}
	gettimeofday(&end, NULL);
	double diff_sec = difftime(end.tv_sec, begin.tv_sec)*1000000;
	double diff_milli = difftime(end.tv_usec, begin.tv_usec);
	printf("send N pack[%d]time:[%f] ", tr_n, (diff_sec+diff_milli)/1000000);
	end:
	return ans;
}

void *recvTR(void *arg)
{
	int ans = 1;
	int exec = -1;
	int i = 0, out1_n = 0, out2_n = 0;
	fd_set rset;
	char b[50];
	struct thr_arg *a = (struct thr_arg *)arg;
	int out1 = a->out1;
	int out2 = a->out2;
	
	int recv = -1;
	
	struct paramThr p;
	p.route_tab = ht_new(50);
	p.buf = b;
	p.bufSize = 50;
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
	
	while((out1_n + out2_n) < tr_n*2) {
		FD_ZERO(&rset);
		FD_SET(out1, &rset);
		FD_SET(out2, &rset);
		exec = select(1024, &rset, NULL, NULL, NULL);
		if(exec == -1) { printf("select err[%s]\n", strerror(errno)); break;}
		else {
			if(FD_ISSET(out1, &rset) == 1) {
				p.sock = out1;
				recv = bo_master_core(&p);
				if(recv == -1) printf("out1 bo_master_core ERROR\n");
				out1_n++;
			}
			if(FD_ISSET(out2, &rset) == 1) {
				p.sock = out2;
				recv = bo_master_core(&p);
				if(recv == -1) printf("out2 bo_master_core ERROR\n");
				out2_n++;
			}
		
		}
	}
	
	gettimeofday(&end, NULL);
	double diff_sec = difftime(end.tv_sec, begin.tv_sec) * 1000000;
	double diff_milli = difftime(end.tv_usec, begin.tv_usec);
	printf("send N pack[%d]time:[%f] ", tr_n*2, (diff_sec + diff_milli)/1000000);
	
	struct timeval tval;
	tval.tv_sec = 1;
	tval.tv_usec = 0;
	FD_ZERO(&rset);
	FD_SET(out1, &rset);
	FD_SET(out2, &rset);
	exec = select(1024, &rset, NULL, NULL, &tval);
	if(exec != 0) {
		printf("ERROR server send more data than wait\n");
		ans = -1;
		goto end;
	}
	printf("select = [%d]\n", exec);
	int find = -1;
	char *key = NULL; char *val = NULL; char *row;
	char ppp[22]; char ddd[22];
	int ii = 0, j = 0; int rrr = 0;
	
	for(i = 0; i < tr_n; i++) {
		rrr = 0;
		row = TR[i]; find = -1;
		memset(ddd, 0, 22);
		memcpy(ddd, row, 21);
		for(j = 0; j <  p.route_tab->size; j++) {
			key = *(p.route_tab->key + j);
			if(key != NULL) {
				memset(ppp, 0, 22);
				val = *(p.route_tab->val + j);
				memcpy(ppp, key, 3);
				ppp[3] = ':';
				memcpy(ppp + 4, val, strlen(val));
				if(strstr(ppp, ddd)) {
					find = 1;
				}
				rrr++;
			}
		}
		if(find != 1) {
			printf(" ERROR don't get from server this row: [");
			for(ii = 0; ii < 21; ii++) {
				printf("%c", TR[i][ii]);
			}
			printf("]\n");
			bo_log(" ===== TAB FROM SERVER ===== ");
			for(j = 0; j < p.route_tab->size; j++) {
				key = *(p.route_tab->key + j);
				if(key != NULL) {
					val = *(p.route_tab->val + j);
					bo_log("%d -> %s:%s", j, key, val);
				}
			}
			bo_log(" ===== END TAB ROUTE   ===== ");
			ans = -1;
		}
	}
	if(rrr != tr_n) {
		printf("ERROR return big tab route\n");
	}
	end:
	return (void *)ans;
}

void *recvTRNULL(void *arg)
{
	int ans = 1;
	int exec = -1;
	int i = 0, out1_n = 0;
	fd_set rset;
	char b[50];
	struct thr_arg *a = (struct thr_arg *)arg;
	int out1 = a->out1;
	
	int recv = -1;
	
	struct paramThr p;
	p.route_tab = ht_new(50);
	p.buf = b;
	p.bufSize = 50;
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
	while(out1_n < tr_n) {
		FD_ZERO(&rset);
		FD_SET(out1, &rset);
		exec = select(out1 + 1, &rset, NULL, NULL, NULL);
		if(exec == -1) { printf("select err[%s]\n", strerror(errno)); break;}
		else {
			if(FD_ISSET(out1, &rset) == 1) {
				p.sock = out1;
				recv = bo_master_core(&p);
				if(recv == -1) printf("out1 bo_master_core ERROR\n");
				out1_n++;
			}		
		}
	}
	gettimeofday(&end, NULL);
	double diff_sec = difftime(end.tv_sec, begin.tv_sec)*1000000;
	double diff_milli = difftime(end.tv_usec, begin.tv_usec);
	printf("send N pack[%d]time:[%f] ", tr_n, (diff_sec+diff_milli)/1000000);
	
	struct timeval tval;
	tval.tv_sec = 1;
	tval.tv_usec = 0;
	FD_ZERO(&rset);
	FD_SET(out1, &rset);
	exec = select(1024, &rset, NULL, NULL, &tval);
	if(exec != 0) {
		printf("ERROR server send more data than wait\n");
		ans = -1;
		goto end;
	}
	
	int find = -1;
	char *key = NULL; char *val = NULL; char *row;
	char ppp[23]; char ddd[9];
	int ii = 0, j = 0; int rrr = 0;
	
	for(i = 0; i < tr_n; i++) {
		rrr = 0;
		row = TR_N[i]; find = -1;
		memset(ddd, 0, 9);
		memcpy(ddd, row, 8);
		for(j = 0; j <  p.route_tab->size; j++) {
			key = *(p.route_tab->key + j);
			if(key != NULL) {
				memset(ppp, 0, 9);
				val = *(p.route_tab->val + j);
				memcpy(ppp, key, 3);
				ppp[3] = ':';
				memcpy(ppp + 4, val, strlen(val));
				if(strstr(ppp, ddd)) {
					find = 1;
				}
				rrr++;
			}
		}
		if(find != 1) {
			printf(" ERROR don't get from server this row: [");
			for(ii = 0; ii < 8; ii++) {
				printf("%c", TR_N[i][ii]);
			}
			printf("]\n");
			ans = -1;
		}
	}
	if(rrr != tr_n) {
		printf("ERROR return big tab route\n");
	}
	end:
	return (void *)ans;
}

int test_toht()
{
	TOHT *tab = ht_new(50);
	int i = 0;
	char key[4];
	char val[18];
	for(; i < tr_n; i++) {
		memset(key, 0, 4);
		memset(val, 0, 18);
		memcpy(key, TR[i], 3);
		memcpy(val, TR[i], 17);
		ht_put(tab, key, val);
	}
	for(i = 0; i < tr_n; i++) {
		
	}
}

int main() 
{
	int blk = -1;
	int in1 = -1, in2 = -1, out1 = -1, out2= -1;
	int exec = -1;
	pthread_t thr1;
	struct thr_arg arg;
	gen_tbl_crc16modbus();
	init_TR();
	printf("STRESS ... RUN \n");
	
	in1 = bo_setConnect("127.0.0.1", 8890);
	if(in1 == -1) { printf("in1 connect ... "); goto error; }
	
	in2 = bo_setConnect("127.0.0.1", 8890);
	if(in2 == -1) { printf("in2 connect ... "); goto error; }

	printf(">CONNECT in1, in2  ... ok\n");

	printf(">SEND TR in1 ... ");
	exec = sendTR(in1);
	if(exec == -1) printf("ERROR\n");
	else printf("OK\n");
	
	
	out1 = bo_setConnect("127.0.0.1", 8891);
	if(out1 == -1) { printf("out1 connect ..."); goto error; }

	out2 = bo_setConnect("127.0.0.1", 8891);
	if(out2 == -1) { printf("out2 connect ..."); goto error; }
	printf(">CONNECT out1, out2 ... ok\n");
	
	arg.out1 = out1;
	arg.out2 = out2;
	
	printf(">RECV ...\n");
	pthread_create(&thr1, NULL, &recvTR, (void *)&arg);
	
	int thr_ans = -1;
	pthread_join(thr1, (void *)&thr_ans);
	if(thr_ans == -1) {
		printf("\nrecvTR ERROR\n");
		goto error;
	}
	printf(">RECV OK \n");

	/* ----------------------------------------- */
	printf("DISCONNECT out2 ...\n");
	bo_closeSocket(out2);
	arg.out2 = -1;
	arg.out1 = out1;
	recvNullTab(&arg);
	
	printf("DISCONNECT out1 ...\n");
	bo_closeSocket(out1);
	printf(">SEND TR in1 ... ");
	exec = sendTR(in1);
	if(exec == -1) printf("ERROR\n");
	else printf("OK\n");
	
	
	out2 = bo_setConnect("127.0.0.1", 8891);
	if(out2 == -1) { printf("out2 error ..."); goto error; }
	printf(">CONNECT out2 ... ok\n");
	
	out1 = bo_setConnect("127.0.0.1", 8891);
	if(out1 == -1) { printf("out2 error ..."); goto error; }
	printf(">CONNECT out1 ... ok\n");
	
	arg.out1 = out1;
	arg.out2 = out2;
	
	printf(">RECV ...\n");
	pthread_create(&thr1, NULL, &recvTR, (void *)&arg);
	
	pthread_join(thr1, (void *)&thr_ans);
	if(thr_ans == -1) {
		printf("\nrecvTR ERROR\n");
		goto error;
	}
	printf(">RECV OK \n");
	
	if(blk == 1) {
		error:
		printf("ERROR\n");
	}
	printf("STRESS ... END \n");
}

void recvNullTab(struct thr_arg *arg) 
{
	printf(">RECV NULL ...\n");
	pthread_t thr1;
	int thr_ans = -1;
	pthread_create(&thr1, NULL, &recvTRNULL, (void *)arg);
	pthread_join(thr1, (void *)&thr_ans);
	if(thr_ans == -1) {
		printf("recvTRNULL ERROR\n");
	}
	printf(">RECV NULL OK\n");
}
/* 0x42 */