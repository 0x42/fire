#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "unity_fixture.h"
#include "../../src/tools/dbgout.h"
#include "../../src/nettcp/bo_net.h"
#include "../../src/nettcp/bo_net_master_core.h"
#include "../../src/tools/oht.h"

TEST_GROUP    (route);

TEST_SETUP    (route) {};
TEST_TEAR_DOWN(route) {};

static int bo_sendAllData_NoSig(int sock, char *buf, int len);
static void *cltSendRoute(void *arg);
static void *cltSendTabRoute(void *arg);
static void *cltSendBadTabRoute(void *arg);


void recvTR(int sock, char **tab, int n);
void sendTR(int s, char **tab, int n);
void test_crtTR(char **tr, int n, int l);

char *MSG[21] = {"003:192.168.100.127:2", "111:192.168.100.102:1"};

TEST(route, simpleTest) /* NEED RUN SERVER */
{
	gen_tbl_crc16modbus();
	printf("simpleTest() ... \n");
	int ans = 1;
	char *msg = ""; //21
	char packet[23] = {0};
	unsigned int n = 21;
	int crc = crc16modbus(*MSG, n);
	unsigned char buf1[2] = {0};
	boIntToChar(crc, buf1);
	
	memcpy(packet, *MSG, 21);
//	printf("\nmsg[%s] packet[%s]\n", msg, packet);
	printf("crc[%d]=[%02x %02x]\n", crc, buf1[0], buf1[1]);
	packet[21] = buf1[0];
	packet[22] = buf1[1];
//	
	ans = bo_sendDataFIFO("127.0.0.1", 8890, packet, sizeof(packet));
	if(ans == -1) {
		printf("can't send value");
	}
	TEST_ASSERT_EQUAL(1, ans);
}

/* нужно переписать тест */
TEST(route, sendgetTest) /* NEED RUN SERVER */
{
	printf("sendgetTest() ...\n");
	struct sockaddr_in saddr;
	struct sockaddr_in saddr2;
	int ans = -1;
	int exec = -1;
	int size = -1;
	int sock_out = -1, sock_in = -1;
	char packet[23] = {0};
	unsigned char txt[2] = {0};
	
	gen_tbl_crc16modbus();
	
	sock_in  = bo_crtSock("127.0.0.1", 8891, &saddr);
	sock_out = bo_crtSock("127.0.0.1", 8890, &saddr2);
	
	exec = connect(sock_out, (struct sockaddr *)&saddr2, sizeof(struct sockaddr));
	if(exec != 0) { 
		printf("can't connect to 8890\n");
		goto end;
	}
	
	exec = connect(sock_in,  (struct sockaddr *)&saddr,  sizeof(struct sockaddr));
	if(exec != 0) {
		printf("can't connect to 8891\n");
		goto end;
	}
	
	char buf[50] = {0};
	exec = t_getMsg(sock_in);
	if(exec == -1) goto end;
	
	memcpy(packet, *(MSG+1), 21);
	int crc = crc16modbus(*(MSG+1), 21);
	boIntToChar(crc, txt);
	
	packet[21] = txt[0];
	packet[22] = txt[1];
	printf("send msg ..");
	exec = bo_sendSetMsg(sock_out, packet, 23);
	if(exec == -1) goto end;
	printf(". ok\n");
	
	exec = t_getMsg(sock_in);
	if(exec == -1) goto end;	
	
	exec = t_getMsg(sock_in);
	if(exec == -1) goto end;
	
	ans = 1;
	
	end:
	close(sock_in );
	close(sock_out);	
	TEST_ASSERT_EQUAL(1, ans);
}

int t_recvSET(int sock, char *buf, int n) 
{
	int count = -1;
	int length = -1;
	int ans = -1;
	count = bo_recvAllData(sock, buf, n, 3);
	if(!strstr(buf, "SET")) {
		printf("t_recvSET() can't get SET errno[%s]\n", strerror(errno));
		goto end;
	}
	
	length = bo_readPacketLength(sock);
	if(length == -1) {
			printf("t_recvSET() can't get Len errno[%s]\n", strerror(errno));
		goto end;
	}
	
	count = bo_recvAllData(sock, buf, n, length);
	if(count == -1) {
		printf("t_recvSET() can't get data errno[%s]\n", strerror(errno));
		goto end;
	}
	
	ans = count;
	end:
	return ans;
}

int t_getMsg(int sock) 
{
	int ans = 1;
	int i,j;
	int exec = -1;
	char buf[50] = {0};
	char packet[21] = {0};
	char *str;
	int n = 2;
	struct timeval tval; 
	tval.tv_sec = 2; 
	tval.tv_usec = 500;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(tval));
	exec = t_recvSET(sock, buf, 50);
	if(exec == -1) {
		printf("t_getMsg() can't recv SET\n");
		return -1;
	}
	
	memcpy(packet, buf, 21);
	int find = -1;
	
	for(i = 0; i < n; i++) {
		str = *(MSG + i);
		if(strstr(packet, str)) { 
			printf("<-[%s]\n", packet);
			find = 1; 
			break; } 
		
	}
	
	if(find == -1) {
		printf("t_getMsg() get bad msg[%s]\n", packet);
		ans = -1;
	}
	
	exec = bo_sendAllData(sock, "OK ", 3);
	if(exec == -1) {
		printf("t_getMsg() cant send OK \n");
		return -1;
	}
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, NULL, sizeof(tval));
	return ans;
}

TEST(route, boMasterCoreTest)
{
	TOHT *tab = ht_new(10);
	struct paramThr p;
	pthread_t thr_cl;
	int res = -1;
	int s_serv = -1;
	int s_cl = -1;
	int ans = -1;
	char buf[30];
	char data[23] = "123:255.254.253.251:1";
	
	printf("boMasterCoreTestf() ... \n");
	s_serv = bo_servStart(9900, 2);
	if(s_serv == -1) {printf("ERR bo_servStart()\n"); goto error; }
	
	res = pthread_create(&thr_cl, NULL, &cltSendRoute, NULL);
	if(res != 0) { printf("ERR can't crt thread"); goto error;}
	
	char *err;
	res = bo_waitConnect(s_serv, &s_cl, &err);
	if(res == -1) {printf("ERR bo_waitConnect()\n");}
	
	p.sock = s_cl;
	p.route_tab = tab;
	p.buf  = buf;
	p.bufSize = sizeof(buf);
	printf("serv try recv SET MSG \n");
	res = bo_master_core(&p);
	if(res == -1) { printf("ERR bo_master_core\n"); goto error;}
	if(res == 1)  { printf("recv SET msg ok \n"); }
	
	int i = 0;
	ans = 1;
	
	for(; i < 21; i++) {
		if(buf[i] != data[i]) ans = -1;
	}
	
	ht_free(tab);
	bo_closeSocket(s_serv);
	error:
	TEST_ASSERT_EQUAL(1, ans);
}

static void *cltSendRoute(void *arg)
{
	gen_tbl_crc16modbus();
	int ans  = -1;
	int s_cl = -1;
	int exec = -1;
	
	char data[23] = "123:255.254.253.251:1";
	int crc = crc16modbus(data, 21);
	unsigned char buf1[2] = {0};
	boIntToChar(crc, buf1);
	data[21] = buf1[0];
	data[22] = buf1[1];
	
	sleep(1);
	s_cl = bo_setConnect("127.0.0.1", 9900);
	if(s_cl == -1) {printf("ERR cltSendRoute bo_setConnect\n"); goto error; }
	
	exec = bo_sendSetMsg(s_cl, data, 23);
	if(exec == -1) {printf("ERR cltSendRoute bo_sendSetMsg");goto error;}
	
	bo_closeSocket(s_cl);
	if(exec == -1) {printf("ERR cltSendRoute bo_closeSocket");goto error;}
	ans = 1;
	error:
	return ans;
}

TEST(route, boMasterCoreTabTest)
{
	printf("boMasterCoreTabTest ... \n");
	bo_log("boMasterCoreTabTest ... RUN");
	TOHT *tab = ht_new(10);
	struct paramThr p;
	pthread_t thr_cl;
	int s_serv = -1, s_cl = -1, res = -1, ans = -1; 
	char buf[BO_MAX_TAB_BUF];
	
	s_serv = bo_servStart(9900, 2);
	if(s_serv == -1) {printf("ERR bo_servStart()\n"); goto error; }
	
	res = pthread_create(&thr_cl, NULL, &cltSendTabRoute, NULL);
	if(res != 0) { printf("ERR can't crt thread"); goto error;}
	
	char *err;
	res = bo_waitConnect(s_serv, &s_cl, &err);
	if(res == -1) {printf("ERR bo_waitConnect()\n");}
	
	p.sock = s_cl;
	p.route_tab = tab;
	p.buf  = buf;
	p.bufSize = sizeof(buf);
	res = bo_master_core(&p);
	if(res == -1) { printf("ERR bo_master_core\n"); goto error;}
	if(res == 1)  { printf("recv TAB msg ok \n"); }
	
	/* Compare data */
	TOHT *tr = ht_new(10);
	char buf_test[BO_MAX_TAB_BUF] = {0};
	char buf_test_recv[BO_MAX_TAB_BUF] = {0};
	ht_put(tr, "000", "NULL");
	ht_put(tr, "001", "192.001.001.002:2");
	ht_put(tr, "002", "255.255.255.255:1");
	ht_put(tr, "003", "192.168.111.001:2");
	ht_put(tr, "004", "NULL");
	
	res = bo_master_crtPacket(tr, buf_test);
	if(res < 1) { printf("ERROR can't crtPacket\n"); goto error;}
	
	
	res = bo_master_crtPacket(p.route_tab, buf_test_recv);
	if(res < 1) { printf("ERROR can't crtPacket from recv data\n"); goto error;}
	
	int ii = 0;
	for(;ii < res; ii++) {
		if(buf_test[ii] != buf_test_recv[ii]) {
			printf("recv bad data \n");
			goto error;
		}
	}
	ans = 1;
	ht_free(tab);
	bo_closeSocket(s_serv);
	error:
	TEST_ASSERT_EQUAL(1, ans);
}

static void *cltSendTabRoute(void *arg)
{
	gen_tbl_crc16modbus();
	int ans  = -1;
	int s_cl = -1;
	int exec = -1;
	TOHT *tr = ht_new(10);
	char buf[BO_MAX_TAB_BUF] = {0};
	ht_put(tr, "000", "NULL");
	ht_put(tr, "001", "192.001.001.002:2");
	ht_put(tr, "002", "255.255.255.255:1");
	ht_put(tr, "003", "192.168.111.001:2");
	ht_put(tr, "004", "NULL");
	
	s_cl = bo_setConnect("127.0.0.1", 9900);
	if(s_cl == -1) {printf("ERR cltSendTabRoute bo_setConnect\n"); goto error; }
	
	exec = bo_master_sendTab(s_cl, tr, buf);
	if(exec == -1) {printf("ERR cltSendTabRoute bo_sendSetMsg");goto error;}
	
	
	ans = 1;
	ht_free(tr);
	
	error:
	bo_closeSocket(s_cl);
	if(exec == -1) {printf("ERR cltSendTabRoute bo_closeSocket");goto error;}
	
	return ans;
}

/* send bad data */
TEST(route, boMasterCoreBadTabTest)
{
	printf("boMasterCoreBadTabTest ... RUN\n");
	bo_log("boMasterCoreBadTabTest ... RUN\n");
	// start SERVER
	int ans = -1, res = -1, s_serv = -1, s_cl = -1;
	struct paramThr p;
	pthread_t thr_cl;
	TOHT *tab = ht_new(10);
	s_serv = bo_servStart(9900, 2);
	char buf[BO_MAX_TAB_BUF];
	if(s_serv == -1) {printf("ERR bo_servStart()\n"); goto error; }
	// Start client
	res = pthread_create(&thr_cl, NULL, &cltSendBadTabRoute, NULL);
	if(res != 0) { printf("ERR can't crt thread"); goto error;}

	char *err;
	res = bo_waitConnect(s_serv, &s_cl, &err);
	if(res == -1) {printf("ERR bo_waitConnect()\n");}

	p.sock = s_cl;
	p.route_tab = tab;
	p.buf  = buf;
	p.bufSize = sizeof(buf);
	res = bo_master_core(&p);
	if(res == -1) { printf("OK\n"); ans = 1;}
	if(res == 1)  { printf("ERR bo_master_core return 1\n"); goto error; }
	
	error:
	TEST_ASSERT_EQUAL(1, ans);
}

static void *cltSendBadTabRoute(void *arg)
{
	gen_tbl_crc16modbus();
	int ans  = -1; int s_cl = -1; int exec = -1;
	unsigned int crc = -1;
	
	unsigned char packet[21] = {0};
	unsigned char value[16] = {0}; // ROW|LEN|NULL|ROW|LENCRC
	unsigned char bb[2] = {0};
	
	memcpy(value, "ROW", 3);
	boIntToChar(4, bb);
	memcpy((value + 3), bb, 2);
	memcpy(value+5, "NULLROW", 7);
	boIntToChar(30, bb);
	memcpy(value+12, bb, 2);
	
	crc = crc16modbus(value, 14);
	boIntToChar(crc, bb);
	memcpy(value+14, bb, 2);
	
	
	
	s_cl = bo_setConnect("127.0.0.1", 9900);
	if(s_cl == -1) {printf("ERR cltSendTabRoute bo_setConnect\n"); goto error; }
	
	exec = bo_sendXXXMsg(s_cl, "TAB", value, 16);
	if(exec == -1) {printf("ERR cltSendTabRoute bo_sendSetMsg");goto error;}
	
	bo_closeSocket(s_cl);
	if(exec == -1) {printf("ERR cltSendTabRoute bo_closeSocket");goto error;}
	ans = 1;
	
	error:
	return ans;
}

TEST(route, sendNULLTest) /* NEED RUN SERVER */
{
	printf("sendNULLTest() ...\n");
	struct sockaddr_in saddr;
	struct sockaddr_in saddr2;
	int ans = -1;
	int exec = -1;
	int size = -1;
	int sock_out = -1, sock_in = -1;
	char packet[10] = {0};
	unsigned char txt[2] = {0};
	char msg_null[8] = "010:NULL";
	
	gen_tbl_crc16modbus();
	
	sock_in  = bo_crtSock("127.0.0.1", 8891, &saddr);
	sock_out = bo_crtSock("127.0.0.1", 8890, &saddr2);
	
	exec = connect(sock_out, (struct sockaddr *)&saddr2, sizeof(struct sockaddr));
	if(exec != 0) { 
		printf("can't connect to 8890\n");
		goto end;
	}
	
	exec = connect(sock_in,  (struct sockaddr *)&saddr,  sizeof(struct sockaddr));
	if(exec != 0) {
		printf("can't connect to 8891\n");
		goto end;
	}
	
	char buf[50] = {0};
//	
	memcpy(packet, msg_null, 8);
	int crc = crc16modbus(msg_null, 8);
	boIntToChar(crc, txt);
	
	packet[8] = txt[0];
	packet[9] = txt[1];
	

	printf("send msg ..");
	exec = bo_sendSetMsg(sock_out, packet, 10);
	if(exec == -1) goto end;
	printf(". ok\n");
	
	int i = 0;
	char buf_out[30] = {0};
	struct paramThr p;
	p.sock = sock_in;
	p.buf  = buf_out;
	p.bufSize = 30;
	p.route_tab = ht_new(50);
	
	bo_master_core(&p);
	
	printf("buf[");
	for(; i < p.length; i++) {
		printf("%c",p.buf[i]);
	}
	printf("]/n");
	ans = 1;
	end:
	close(sock_in );
	close(sock_out);	
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(route, sendLogTest) /* NEED SERVER */
{
	printf("sendLogTest ... RUN\n");
	int ans = -1, in = -1, exec = -1;
	char *log = "0x42: test log message";
	char packet[24] = {0};
	unsigned char crcTxt[2] = {0};
	int crc = 0;
	
	gen_tbl_crc16modbus();
		
	in = bo_setConnect("127.0.0.1", 8890);
	if(in == -1) {
		printf("bo_setConnect ERROR\n"); goto exit;
	}
	
	crc = crc16modbus(log, strlen(log));
	boIntToChar(crc, crcTxt);
	
	memcpy(packet, log, 22);
	
	packet[22] = crcTxt[0];
	packet[23] = crcTxt[1];
	
	exec = bo_sendLogMsg(in, packet, 24);
	if(exec == -1) {
		printf("bo_sendLogMsg() ERROR\n");
	}
	
	ans = 1;
	exit:
	TEST_ASSERT_EQUAL(1, ans);
}

/*
 * тест на ошибки  
 */
TEST(route, closeTest)
{
	const int error = -1;
	int s_cl = -1;
	int exec = -1;
	struct sockaddr_in saddr;
	
	printf("start client ..... \n");
	s_cl = bo_crtSock("192.168.1.11", 8123, &saddr);
	if(s_cl == -1) {
		printf("bo_crtSock() errno[%s]\n", strerror(errno));
		goto error;
	}
	
	int i = 1;
	setsockopt( s_cl, IPPROTO_TCP, TCP_NODELAY, (void *)&i, sizeof(i));
	
	exec = connect(s_cl, (struct sockaddr *)&saddr, sizeof(struct sockaddr));
	if(exec != 0) {
		printf("connect() errno[%s]\n", strerror(errno));
		close(s_cl);
		goto error;
	}
	
	printf("send msg ...");
	char msg[3] = "T01";
	exec = bo_sendAllData_NoSig(s_cl, msg, 3);
	if(exec == -1) {
		printf("bo_sendAllData() errno[%s]\n", strerror(errno));
		goto error;
	}
	printf(" ..ok[%d]\n", exec);
	
	sleep(20);
	printf("send msg2 ...");
	char msg2[3] = "T02";
	exec = bo_sendAllData_NoSig(s_cl, msg2, 3);
	if(exec == -1) {
		printf("bo_sendAllData() errno[%s]\n", strerror(errno));
		goto error;
	}
	printf(" ..ok[%d]\n", exec);
	
	sleep(20);
	printf("send msg3 ...");
	char msg3[3] = "T03";
	exec = bo_sendAllData_NoSig(s_cl, msg3, 3);
	if(exec == -1) {
		printf("bo_sendAllData() errno[%s]\n", strerror(errno));
		goto error;
	}
	printf(" ..ok[%d]\n", exec);
	sleep(20);
	
	close(s_cl);
	if(error == 1) {
error:
		printf("ERROR\n");
	}
	printf("end\n");
}

/* send with blck signal */
static int bo_sendAllData_NoSig(int sock, char *buf, int len)
{
	int count = 0;   /* кол-во отпр байт за один send*/
	int allSend = 0; /* кол-во всего отправл байт*/
	unsigned char *ptr = buf;
	int n = len;
	int i = 1;
	int ni = sizeof(i);
	int err = getsockopt(sock, SOL_SOCKET, SO_ERROR, &i, &ni);
	if(err < 0) {
		printf("getsockopt() err[%s]\n", strerror(errno));
	} 
		printf("getsockopt() i[%d][%s]\n",i, strerror(i));
	while(allSend < len) {
		/* блок возм сигнал SIGPIPE */
		count = send(sock, ptr + allSend, n - allSend, MSG_NOSIGNAL);
		if(count == -1) break;
		allSend += count;
	}
	return (count == -1 ? -1 : allSend);	
}

/*
0   0  3  :    1  6  8  .   1  0  0  .   1  2  7   :  2
48 48 51 58   49 54 56 46  49 48 48 46  49 50 55  58 50 166 196

003:192.

*/















