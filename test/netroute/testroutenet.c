#include <string.h>
#include "unity_fixture.h"
#include "../../src/tools/dbgout.h"
#include "../../src/nettcp/bo_net.h"
TEST_GROUP    (route);

TEST_SETUP    (route) {};
TEST_TEAR_DOWN(route) {};

TEST(route, simpleTest)
{
	gen_tbl_crc16modbus();
	printf("simpleTest() ... \n");
	int ans = 1;
	char *msg = "110:192.168.100.100:2"; //21
	char packet[23] = {0};
	unsigned int n = 21;
	int crc = crc16modbus(msg, n);
	unsigned char buf1[2] = {0};
	boIntToChar(crc, buf1);
	
	memcpy(packet, msg, 21);
	printf("\nmsg[%s] packet[%s]\n", msg, packet);
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
	char *msg = "110:192.168.100.100:2";
	
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
	
	
	memcpy(packet, msg, 21);
	int crc = crc16modbus(msg, 21);
	boIntToChar(crc, txt);
	
	packet[21] = txt[0];
	packet[22] = txt[1];

	exec = bo_sendSetMsg(sock_out, packet, 23);
	if(exec == -1) goto end;
	printf("send msg ... ok\n");
	
	char buf[50] = {0};
	exec = t_recvSET(sock_in, buf, 50);
	
	printf("buf[");
	int i, j = 0;
	for(i = 0; i < 21; i++) {
		printf("%c", buf[i]);
	}
	printf("] msg[%s]\n", msg);
	
	ans = -1;
	for(i = 0; i < 21; i++) {
		if(buf[i] != msg[i]) {
			printf("bad msg recv\n");
			printf("msg [%s] buf[", msg);
			for(j = 0; j < exec; j++) {
				printf("%c", buf[j]);
			}
			printf("]\n");
			goto end;
		}
	}
	exec = bo_sendAllData(sock_in, "OK ", 3);
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

	sock_in  = bo_crtSock("127.0.0.1", 8891, &saddr);
	sock_out = bo_crtSock("127.0.0.1", 8890, &saddr2);

	s2 = bo_crtSock("127.0.0.1", 8891, &s2_a);
	s3 = bo_crtSock("127.0.0.1", 8891, &s3_a);
	s4 = bo_crtSock("127.0.0.1", 8891, &s4_a);
	s5 = bo_crtSock("127.0.0.1", 8891, &s5_a);
	
	exec = connect(sock_out, (struct sockaddr *)&saddr2, sizeof(struct sockaddr));
	if(exec != 0) printf("can't connect to 8890\n");
	
	exec = connect(sock_in,  (struct sockaddr *)&saddr,  sizeof(struct sockaddr));
	if(exec != 0) printf("can't connect to 8891\n");
	exec = connect(s2,  (struct sockaddr *)&s2_a,  sizeof(struct sockaddr));
	if(exec != 0) printf("can't connect to 8891\n");
	exec = connect(s3,  (struct sockaddr *)&s3_a,  sizeof(struct sockaddr));
	if(exec != 0) printf("can't connect to 8891\n");
	exec = connect(s4,  (struct sockaddr *)&s4_a,  sizeof(struct sockaddr));
	if(exec != 0) printf("can't connect to 8891\n");
	exec = connect(s5,  (struct sockaddr *)&s5_a,  sizeof(struct sockaddr));
	if(exec != 0) printf("can't connect to 8891\n");
	
	
	/* Получаем данные которые уже есть в таблице и сравниваем*/
	exec = t_getMsg(sock_in, msg);
	if(exec == -1) { printf("error when recv data from tab \n"); goto end; }
	exec = t_getMsg(s2, msg);
	if(exec == -1) { printf("error when recv data from tab \n"); goto end; }
	exec = t_getMsg(s3, msg);
	if(exec == -1) { printf("error when recv data from tab \n"); goto end; }
	exec = t_getMsg(s4, msg);
	if(exec == -1) { printf("error when recv data from tab \n"); goto end; }
	exec = t_getMsg(s5, msg);
	if(exec == -1) { printf("error when recv data from tab \n"); goto end; }
	
	
	
	memcpy(packet, msg2, 21);
	int crc = crc16modbus(msg2, 21);
	boIntToChar(crc, txt);
	
	packet[21] = txt[0];
	packet[22] = txt[1];

	exec = bo_sendSetMsg(sock_out, packet, 23);
	
	if(exec == -1) goto end;
	printf("send msg ... ok\n");
	
	exec = t_getMsg(sock_in, msg);
	if(exec == -1) { printf("error when recv msg2 from tab \n"); goto end; }
	exec = t_getMsg(s2, msg);
	if(exec == -1) { printf("error when recv msg2 from tab \n"); goto end; }
	exec = t_getMsg(s3, msg);
	if(exec == -1) { printf("error when recv msg2 from tab \n"); goto end; }
	exec = t_getMsg(s4, msg);
	if(exec == -1) { printf("error when recv msg2 from tab \n"); goto end; }
	exec = t_getMsg(s5, msg);
	if(exec == -1) { printf("error when recv msg2 from tab \n"); goto end; }
	
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
		goto end;
	}
	
	length = bo_readPacketLength(sock);
	if(length == -1) goto end;
	
	count = bo_recvAllData(sock, buf, n, length);
	if(count == -1) goto end;
	
	ans = count;
	end:
	return ans;
}

int t_getMsg(int sock, char *msg) 
{
	int ans = 1;
	int i,j;
	int exec = -1;
	char buf[50] = {0};
	struct timeval tval; tval.tv_sec = 0; tval.tv_usec = 100;
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(tval));
	exec = t_recvSET(sock, buf, 50);
	if(exec == -1) {
		printf("t_getMsg() can't recv SET\n");
		return -1;
	}
	for(i = 0; i < 21; i++) {
		if(buf[i] != msg[i]) {
			printf("bad msg recv\n");
			printf("msg [%s] buf[", msg);
			for(j = 0; j < exec; j++) {
				printf("%c", buf[j]);
			}
			printf("]\n");
			return -1;
		}
	}
	
	exec = bo_sendAllData(sock, "OK ", 3);
	if(exec == -1) {
		printf("t_getMsg() cant send OK \n");
		return -1;
	}
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, NULL, sizeof(tval));
	return ans;
}