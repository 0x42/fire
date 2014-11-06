#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "unity_fixture.h"
#include "../../src/tools/dbgout.h"
#include "../../src/nettcp/bo_net.h"
TEST_GROUP    (route);

TEST_SETUP    (route) {};
TEST_TEAR_DOWN(route) {};

char *MSG[21] = {"110:192.168.100.101:2", "111:192.168.100.102:1"};

TEST(route, simpleTest)
{
	gen_tbl_crc16modbus();
	printf("simpleTest() ... \n");
	int ans = 1;
	char *msg = "110:192.168.100.101:2"; //21
	char packet[23] = {0};
	unsigned int n = 21;
	int crc = crc16modbus(msg, n);
	unsigned char buf1[2] = {0};
	boIntToChar(crc, buf1);
	
	memcpy(packet, *MSG, 21);
//	printf("\nmsg[%s] packet[%s]\n", msg, packet);
//	printf("crc[%d]=[%02x %02x]\n", crc, buf1[0], buf1[1]);
	packet[21] = buf1[0];
	packet[22] = buf1[1];
//	
	ans = bo_sendDataFIFO("127.0.0.1", 8890, packet, sizeof(packet));
	if(ans == -1) {
		printf("can't send value");
	}
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(route, getTest)
{
	printf("getTest ... \n");
	int ans = 1;
	int exec = -1;
	struct sockaddr_in saddr;
	int sock = bo_crtSock("127.0.0.1", 8891, &saddr);
	if(sock < 1) ans = -1;
	exec = connect(sock, (struct sockaddr *)&saddr, sizeof(struct sockaddr));
	if(exec != 0) printf("can't connect to 8891\n");
	
	sleep(1);
	bo_closeSocket(sock);
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(route, sendgetTest)
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
	bo_log("send msg ... test");
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


TEST(route, send1get5Test)
{
	printf("send1get5Test() ...\n");
	struct sockaddr_in saddr;
	struct sockaddr_in saddr2, s2_a, s3_a, s4_a, s5_a;
	int ans = -1;
	int exec = -1;
	int size = -1;
	int sock_out = -1, sock_in = -1;
	int s2, s3, s4, s5;
	
	char packet[23] = {0};
	unsigned char txt[2] = {0};
	char *msg  = "110:192.168.100.100:2";
	char *msg2 = "123:192.168.123.123:1";
	char *tab1[21] = {"110:192.168.100.100:2"};
	char *tab2[21] = {"110:192.168.100.100:2", "123:192.168.123.123:1"};
	
	sock_in  = bo_crtSock("127.0.0.1", 8891, &saddr);
	sock_out = bo_crtSock("127.0.0.1", 8890, &saddr2);

	exec = connect(sock_out, (struct sockaddr *)&saddr2, sizeof(struct sockaddr));
	if(exec != 0) printf("can't connect to 8890\n");
	
	exec = connect(sock_in,  (struct sockaddr *)&saddr,  sizeof(struct sockaddr));
	if(exec != 0) printf("can't connect to 8891\n");
	
	/* Получаем данные которые уже есть в таблице и сравниваем */
	exec = t_getMsg(sock_in, tab1, 1);
	if(exec == -1) { printf("error when s_in recv data from tab \n"); goto end; }
	
	memcpy(packet, msg2, 21);
	int crc = crc16modbus(msg2, 21);
	boIntToChar(crc, txt);
	
	packet[21] = txt[0];
	packet[22] = txt[1];

	exec = bo_sendSetMsg(sock_out, packet, 23);
	
	if(exec == -1) goto end;
	printf("send msg ... ok\n");
	
	exec = t_getMsg(sock_in, tab2, 2);
	if(exec == -1) { printf("error when sock_in recv msg2 from tab \n"); goto end; }
	exec = t_getMsg(sock_in, tab2, 2);
	if(exec == -1) { printf("error when sock_in recv msg2-2 from tab \n"); goto end; }

	ans = 1;
	end:
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
	s_cl = bo_crtSock("127.0.0.1", 8123, &saddr);
	if(s_cl == -1) {
		printf("bo_crtSock() errno[%s]\n", strerror(errno));
		goto error;
	}
	
	exec = connect(s_cl, (struct sockaddr *)&saddr, sizeof(struct sockaddr));
	if(exec != 0) {
		printf("connect() errno[%s]\n", strerror(errno));
		close(s_cl);
		goto error;
	}
	
	printf("send msg ...");
	char msg[3] = "T01";
	exec = bo_sendAllData(s_cl, msg, 3);
	if(exec == -1) {
		printf("bo_sendAllData() errno[%s]\n", strerror(errno));
		goto error;
	}
	printf(" ..ok[%d]\n", exec);
//	sleep(5);
//	printf("send msg2 ...");
//	char msg2[3] = "T02";
//	exec = bo_sendAllData(s_cl, msg2, 3);
//	if(exec == -1) {
//		printf("bo_sendAllData() errno[%s]\n", strerror(errno));
//		goto error;
//	}
//	printf(" ..ok[%d]\n", exec);
	
	close(s_cl);
	if(error == 1) {
error:
		printf("ERROR\n");
	}
	printf("end\n");
}





























