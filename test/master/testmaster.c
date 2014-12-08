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

static void *cltSendRoute(void *arg);
static void *cltSendTabRoute(void *arg);
static void *cltSendBadTabRoute(void *arg);

TEST_GROUP(master);

TEST_SETUP    (master) {}
TEST_TEAR_DOWN(master) {}

/* ----------------------------------------------------------------------------
 * Проверка КА при приеме SET сообщения 
 */
TEST(master, boMasterCoreTest)
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

/* ----------------------------------------------------------------------------
 * отправка нескольких сообщений. Проверка достоверности принятых данных. 
 */
TEST(master, boMasterCoreTabTest)
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

/* ----------------------------------------------------------------------------
 * Отправка некорректного пакета 
 */
TEST(master, boMasterCoreBadTabTest)
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

/* ----------------------------------------------------------------------------
 * 
 */
TEST(master, sendNULLTest) /* NEED RUN SERVER */
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
	
	int i = 0;
	char buf_out[30] = {0};
	struct paramThr p;
	p.sock = sock_in;
	p.buf  = buf_out;
	p.bufSize = 30;
	p.route_tab = ht_new(50);
	
	bo_master_core(&p);
	
	ans = 1;
	end:
	close(sock_in );
	close(sock_out);	
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(master, chkSockTest) /* NEED RUN SERVER */
{
	printf("chkSockTest() ... run\n");
	int ans = -1, exec = -1;
	int sock = -1;
	
	sock = bo_setConnect("127.0.0.1", 8890);
	if(sock == -1) {
		printf("bo_setConnect ERROR\n"); goto exit;
	}
	
	exec = bo_chkSock(sock);
	if(exec == 1) ans = 1;
	else {
		printf("bo_chkSock() ERROR\n");
	}
	
	exit:
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(master, sendLogTest) /* NEED RUN SERVER */
{
	printf("\nsendLogTest ... RUN\n");
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

TEST(master, sendGetLogTest) /* NEED RUN SERVER */
{
	int ans = -1;
	printf("sendGetLogTest ... run \n");
	char *log = "0x42: test log message";
	
	int in = bo_setConnect("127.0.0.1", 8890);
	if(in == -1) { printf("bo_setConnect ERROR\n"); goto exit; }
	
	char buf[BO_ARR_ITEM_VAL] = {0};
	
	int exec = bo_master_core_logRecv(in, 0, buf, BO_ARR_ITEM_VAL);
	if(exec == -1) { printf("bo_master_core_logRecv() ERROR\n"); goto exit;}
	int i = 0;
	if(exec != strlen(log)) { printf("bad exec[%d]!=len[%d] ERROR\n", exec, strlen(log)); goto exit;}
	
	for(; i < strlen(log); i++) {
		if(buf[i] != *(log+i) ) { 
			printf("recv bad log ERROR \n"); goto exit;
		}
	}
	ans = 1;
	exit:
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(master, getNullLogTest) /* NEED RUN SERVER */
{
	int ans = -1;
	printf("getNullLogTest ... run \n");
	int in = bo_setConnect("127.0.0.1", 8890);
	if(in == -1) { printf("bo_setConnect ERROR\n"); goto exit; }
	
	char buf[BO_ARR_ITEM_VAL] = {0};
	
	int exec = bo_master_core_logRecv(in, 100, buf, BO_ARR_ITEM_VAL);
	if(exec == -1) { printf("bo_master_core_logRecv() ERROR\n"); goto exit;}
	else if(exec > 0) {printf("bo_master_core_logRecv() ERROR\n"); goto exit;}


	ans = 1;
	exit:
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(master, set2000getLast) /* NEED RUN SERVER */
{
	int ans = -1;
	printf("\n set2048getLast ... run \n");
	char log[2000][1100] = {0};
	int i = 0;
	char *str;
	
	for(i = 0; i < 2000; i++) {
		str = log[i];
		sprintf(str, "%d %s", i, "0x42 .....");
	}
		
	int in = bo_setConnect("127.0.0.1", 8890);
	if(in == -1) { printf("bo_setConnect ERROR\n"); goto exit; }
	
	int crc; unsigned char crcTxt[2] = {0}; 
	char buf[BO_ARR_ITEM_VAL] = {0};
	
	int len; int exec = -1;
	for(i = 0; i < 2000; i++) {
		len = strlen(log[i]);

		crc = crc16modbus(log[i], len);
		boIntToChar(crc, crcTxt);

		memcpy(buf, log[i], len);
		buf[len] = crcTxt[0];
		buf[len + 1] = crcTxt[1];

		exec = bo_sendLogMsg(in, buf, len + 2);
		if(exec == -1) {
			printf("bo_sendLogMsg() ERROR\n");
			goto exit;
		}
	}
	//975 
	exec = bo_master_core_logRecv(in, 975, buf, BO_ARR_ITEM_VAL);
	if(exec == -1) { printf("bo_master_core_logRecv() ERROR\n"); goto exit; }
	
	printf("buf[");
	for(i = 0; i < exec; i++) {
		printf("%c", buf[i]);
	}
	printf("\n");
	
	if(exec != strlen(log[1999])) { printf("bad exec[%d]!=len[%d] ERROR\n", exec, strlen(log)); goto exit;}
	
	printf("log[1999][");
	for(i = 0; i<strlen(log[1999]); i++) {
		printf("%c", log[1999][i]);
	}
	printf("\n");
	
	
	
	for(i = 0; i < strlen(log[1999]); i++) {
		if(buf[i] != log[1999][i] ) { 
			printf("recv bad log ERROR \n"); goto exit;
		}
	}
	
	ans = 1;
	exit:
	TEST_ASSERT_EQUAL(1, ans);
}

/* 0x42 */