#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <pthread.h>
#include <time.h>
#include <mcheck.h>

#include "../../../src/tools/ocrc.h"
#include "../../../src/tools/oht.h"
#include "../../../src/nettcp/bo_net_master_core.h"

char TR[200][23];
char TR_N[200][10];
int tr_n = 200;

TOHT *HT_TR;
int ht_N = 200;

char *ip_master = "127.0.0.1";

struct thr_arg {
	int out1;
	int out2;
}; 

/* ----------------------------------------------------------------------------
 * Иниц-ия табл роутов 
 */
void initHTTR()
{
	HT_TR = ht_new(10);
	int i = 0;
	char key[4];
	for(; i < ht_N; i++) {
		memset(key, 0, 4);
		sprintf(key, "%03d", i);
		ht_put(HT_TR, key, "192.168.001.001:1");
	}

}

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
		sprintf(str, "%03d:192.168.001.001:1", i);
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

/* -------------------------------------------------------------------------- */
/* Отправка построчно таблицы роутов на сервер */
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

/* ----------------------------------------------------------------------------
 * Получение таблицы роутов с сервера одним пакетом
 */
void *recvOnePacket(void *arg)
{
	int ans = 1;
	int exec = -1;
	int i = 0, out1_n = 0, out2_n = 0;
	fd_set rset;
	struct thr_arg *a = (struct thr_arg *)arg;
	int out1 = a->out1;
	char bb2[BO_MAX_TAB_BUF]= {0};
	int recv = -1;
	
	struct paramThr p;
	p.route_tab = ht_new(50);
	p.buf = bb2;
	p.bufSize = BO_MAX_TAB_BUF;
	printf("RECV IN ONE PACKET ... \n");
	struct timeval begin, end;
	gettimeofday(&begin, NULL);
	
	FD_ZERO(&rset);
	FD_SET(out1, &rset);
	exec = select(1024, &rset, NULL, NULL, NULL);
	if(exec == -1) { printf("select err[%s]\n", strerror(errno)); goto end;}
	else {
		if(FD_ISSET(out1, &rset) == 1) {
			p.sock = out1;
			recv = bo_master_core(&p);
			if(recv == -1) printf("out1 bo_master_core ERROR\n");
		}
	}
	
	gettimeofday(&end, NULL);
	double diff_sec = difftime(end.tv_sec, begin.tv_sec) * 1000000;
	double diff_milli = difftime(end.tv_usec, begin.tv_usec);
	printf("\nRECV N pack[%d]time:[%f] \n", 1, (diff_sec + diff_milli)/1000000);
	
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
	char bb1[BO_MAX_TAB_BUF]= {0};
	
	exec = bo_master_crtPacket(HT_TR, bb1);
	if(exec < 1) {printf("ERROR bad tab TH_TR"); goto end;}

	exec = bo_master_crtPacket(p.route_tab, bb2);
	if(exec < 1) {printf("ERROR bad recv tab "); goto end;}	
	
	int ii = 0;
	for(;ii < exec; ii++) {
		if(bb1[ii]!= bb2[ii]) { 
			printf("ERROR recv bad tab\b"); 
			ans = -1; goto end;}
	}
	end:
	return (void *)ans;
}

void send_tab_in_one_packet() 
{
	printf("STRESS SEND TAB IN SEND \n");
	char buf[BO_MAX_TAB_BUF] = {0};
	int len = 0; int i = 0; int exec = -1;
	const int error = 1;
	pthread_t thr1;
	struct thr_arg arg;
	gen_tbl_crc16modbus();
	init_TR();
	initHTTR();
		
	int in1 = -1, out1 = -1;
	in1 = bo_setConnect(ip_master, 8890);
	if(in1 == -1) { printf("in1 connect ... "); goto error; }
	
	printf(">SEND ");
	exec = sendTR(in1);
	if(exec == -1) { printf("sendTR ...."); goto error;}
	printf(" ... OK\n");
	
	
	out1 = bo_setConnect(ip_master, 8891);
	if(out1 == -1) { printf("out1 connect ... "); goto error; }
	arg.out1 = out1;
	arg.out2 = -1;
	
	printf(">RECV ...\n");
	pthread_create(&thr1, NULL, &recvOnePacket, (void *)&arg);
	
	int thr_ans = -1;
	pthread_join(thr1, (void *)&thr_ans);
	if(thr_ans == -1) {
		printf("\nrecvTR ERROR\n");
		goto error;
	}
	printf(">RECV OK \n");
	
	if(error == -1) {
		error:
		printf("ERROR\n");
	}
	printf("END STRESS\n");
}

void test_print_tab(TOHT *tab)
{
	char *kk; char *vv;
	printf("==== TAB ROUTE ====\n");
	int j;
	for(j = 0; j < tab->size; j++ ) {
		kk = *(tab->key + j);
		if(kk != NULL) {
			vv = *(tab->val + j);
			printf("[%s:%s]\n", kk, vv);
		}
	}
	printf("==== END TAB ====\n");

}

int main() 
{
	send_tab_in_one_packet();
}

/* 0x42 */