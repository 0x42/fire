#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "../../src/nettcp/bo_fifo.h"
#include "../../src/tools/ocrc.h"

#include "unity_fixture.h"

TEST_GROUP(fifo);

TEST_SETUP(fifo) {}

TEST_TEAR_DOWN(fifo) {}

static struct sockaddr_in saddr;
static int startSock()
{
	int ans = 1;
	struct in_addr servip;
	int sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock != -1) {
		saddr.sin_family = AF_INET;
		saddr.sin_port = htons(8888);
		inet_aton("127.0.0.1", &servip);
		saddr.sin_addr.s_addr = servip.s_addr;
		ans = sock;
	} else ans = -1;
	return ans;
}

void boIntToChar(unsigned int x, unsigned char *ans)
{
	unsigned int a = 0;
	a = x >> 8;
	a = a & 0xFF;
	*ans = (char)a;
	a = x & 0xFF;
	*(ans + 1) = (char)a;
}

unsigned int boCharToInt(unsigned char *buf) 
{
	unsigned int x = 0;
	x = x | buf[0];
	x = x << 8;
	x = x & 0xFF00;
	x = x | buf[1];
	/*делаем маску выделяем последние 2 байта*/
	x = x & 0xFFFF;
	return x;
}

TEST(fifo, sendOneByte)
{
	printf("sendOneByte() ... \n");
	int ans = 0;
	int sock = startSock();
	unsigned char *msg = "A"; 
	char buf[3] = {0};
	if(sock == -1) {
		printf("startSock() err%s\n",strerror(errno));
		ans = 0;
	} else {
		if (connect(sock, (struct sockaddr *) &saddr, 
		sizeof(struct sockaddr)) == 0) {
			int exec = send(sock, (void*)msg, 1, 0);
			if(exec != -1)	{
				exec = recv(sock, buf, sizeof(buf), 0);
				if(exec > 0) {
					int p = strstr(buf, "ERR");
					if(p) ans = 1;
				} else {
					printf("not get ERR");
				}
			}
			else printf("send() err[%s]\n", strerror(errno));
		} else {
			printf("connect() err[%s]\n", strerror(errno));
		}
	}
	if(close(sock) == -1) {
		printf("close() err%s\n",strerror(errno));
		ans = 0;
	}
	TEST_ASSERT_EQUAL(1, ans);
}
/* ----------------------------------------------------------------------------
 * @brief	отправляем сообщение SET length=10 msg size = 10
 */
int sendMSG(unsigned char *msg, int msgSize, int length) 
{
	char *head = "SET";
	char len[2] = {0};
	char buf[3] = {0};
	boIntToChar(length, len);
	int ans = -1;
	int sock = startSock();
	if(sock == -1) {
		printf("startSock() err%s\n",strerror(errno));
	} else {
		if (connect(sock, (struct sockaddr *) &saddr, 
		sizeof(struct sockaddr)) == 0) {
			int exec = send(sock, (void*)head, 3, 0);
			if(exec == -1)	{ printf("send head err\n"); goto error;}
			exec = send(sock, (void*)len, 2, 0 );
			if(exec == -1)	{ printf("send length err\n"); goto error;}
			exec = send(sock, (void*)msg, msgSize, 0 );
			if(exec == -1)	{ printf("send msg err\n"); goto error;}
			exec = recv(sock, buf, sizeof(buf), 0);
			if(exec > 0) {
				int p = strstr(buf, "OK");
				if(p) ans = 1;
			} else {
				printf("send() err[%s]\n", strerror(errno)); 
				goto error;
			}
		} else {
			printf("connect() err[%s]\n", strerror(errno));
		}
	}
	error:
	if(close(sock) == -1) {
		printf("close() err%s\n",strerror(errno));
		ans = -1;
	}
	return ans;
}
unsigned char *getMSG(int *s, int *retLength)
{
	unsigned char *buf = NULL;
	char *head = "GET";
	char h[3] = {0};
	char l[2] = {0};
	unsigned int length = 0;
	int total = 0, i = 0;
	int sock = startSock();
	if(sock == -1) {
		printf("getMSG() startSock() err%s\n",strerror(errno));
	} else {
		if (connect(sock, (struct sockaddr *) &saddr, 
		sizeof(struct sockaddr)) == 0) {
			*s = sock;
			int count = send(sock, head, 3, 0);
			if(count != 3) goto exit;
			count = recv(sock, h, 3, 0);
			if(count != 3) goto exit;
			if(strstr(h, "VAL")) {
				count = recv(sock, l, 2, 0);
				if(count != 2) goto exit;
				length = boCharToInt(l);
				*retLength = length;
				buf = (unsigned char *)malloc(length);
				if(buf == NULL) goto exit;
				total = 0;
				while(total < length) {
					count = recv(sock, buf+total, length-total, 0);
					if(count < 0) goto exit;
					total += count;
				}
			} else {
				buf = (unsigned char *)malloc(3);
				memcpy(buf, h, 3);
				*retLength = 3;
			}
		} else {
			printf("getMSG() connect() err[%s]\n", strerror(errno));
		}
	}
	exit:
	return buf;
}

TEST(fifo, sendSETL10MSG10)
{
	printf("sendSETL10MSG10() ... \n");
	int ans = 0;
	int i = 0;
	int exec = 0;
	int sock;
	int bufSize = 0;
	int msgSize = 10;
	unsigned char *msg = "AAAAAAAAAA";
	unsigned char *buf = NULL;
	unsigned char STA[4] = {0};
	ans = sendMSG(msg, msgSize, msgSize);
	if(ans != 1) goto error;
	buf = getMSG(&sock, &bufSize);
	if(buf != NULL) {
		if(msgSize == bufSize) {
			for(i = 0; i < msgSize; i++) {
				if(msg[i] != buf[i]) goto error;
			}
			ans = 1;
		}
	}
	exec = send(sock, "DEL", 3, 0);
	if(exec == -1) {
		exec = recv(sock, STA, 3, 0);
		STA[4] = "\0";
		if(exec != -1) printf("STA = %s\n", STA);
		ans = 0; 
	}
	close(sock);
	error:
	if(buf != NULL) free(buf);
	TEST_ASSERT_EQUAL(1, ans);
}
/* ----------------------------------------------------------------------------
 * @brief	отправляем сообщение SET length=10 msg size = 9
 */
TEST(fifo, sendSETL10MSG9)
{
	printf("sendSETL10MSG9() ... \n");
	int ans = 0;
	int exec = 1;
	int sock;
	unsigned char *msg = "AAAAAAAAA";
	unsigned char *buf;
	int bufSize = 0;
	int msgSize = 9;
	int length = 10;
	ans = sendMSG(msg, msgSize, length);
	if(ans == 1) goto error;
	buf = getMSG(&sock, &bufSize);
	close(sock);
	if(buf != NULL) {
		printf("		buf != NULL\n size=%d\n", bufSize);
		if(strstr(buf, "NO")) ans = 1;
		free(buf);
	}
	if(exec == -1) {
		error:
		ans = 0;
	}
	TEST_ASSERT_EQUAL(1, ans);
}
/* ----------------------------------------------------------------------------
 * @brief	отправляем сообщение SET length=10 msg size = 23
 */
TEST(fifo, sendSETL10MSG23)
{
	printf("sendSETL10MSG23() ... \n");
	int ans = 0;
	int sock = startSock();
	int exec = 0, p = 0;
	int tcp_delay = 1;
	char *head = "SET";
	unsigned char *msg = "AAAAA AAAAA AAAAA AAAAA"; 
	char len[2] = {0};
	char buf[4] = {0};
	boIntToChar(10, len);
	if(sock == -1) {
		printf("startSock() err%s\n",strerror(errno));
		ans = 0;
	} else {
		int e = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &tcp_delay, sizeof(tcp_delay));
		if (e == -1) {
			printf("setsockopt errno[%s]\n", strerror(errno));
			goto error;
		}
		if (connect(sock, (struct sockaddr *) &saddr, 
		sizeof(struct sockaddr)) == 0) {
			int exec = send(sock, (void*)head, 3, 0);
			if(exec == -1)	{ printf("send head err\n"); goto error;}
			exec = send(sock, (void*)len, 2, 0 );
			if(exec == -1)	{ printf("send length err\n"); goto error;}
			exec = send(sock, (void*)msg, 23, 0 );
			if(exec == -1)	{ printf("send msg err\n"); goto error;}
			if(exec < 23) { printf("cant send all msg\n"); goto error;}
			exec = recv(sock, buf, sizeof(buf), 0);
			buf[4] = "\0";
			printf("STA = %s\n", buf);
			if(exec > 0) {
				p = strstr(buf, "ERR");
				if(p) ans = 1;
			} else {printf("send() err[%s]\n", strerror(errno)); goto error;}
		} else {
			printf("connect() err[%s]\n", strerror(errno));
		}
	}
	if(close(sock) == -1) {
		printf("close() err%s\n",strerror(errno));
		ans = 0;
	}
	error:
	if(exec == -1) {
		ans = 0;
	}
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(fifo, sendOnlyHead)
{
	int ans = 0;
	int sock = startSock();
	int exec = 0;
	char *head = "SET";
	unsigned char *msg = "AAAAAAAAA"; 
	char len[2] = {0};
	char buf[3] = {0};
	boIntToChar(10, len);
	if(sock == -1) {
		printf("startSock() err%s\n",strerror(errno));
		ans = 0;
	} else {
		if (connect(sock, (struct sockaddr *) &saddr, 
		sizeof(struct sockaddr)) == 0) {
			int exec = send(sock, (void*)head, 3, 0);
			if(exec == -1)	{ printf("send head err\n"); goto error;}
			exec = recv(sock, buf, sizeof(buf), 0);
			if(exec > 0) {
				int p = strstr(buf, "ERR");
				if(p) ans = 1;
			} else {printf("send() err[%s]\n", strerror(errno)); goto error;}
		} else {
			printf("connect() err[%s]\n", strerror(errno));
		}
	}
	if(close(sock) == -1) {
		printf("close() err%s\n",strerror(errno));
		ans = 0;
	}
	error:
	if(exec == -1) {
		ans = 0;
	}
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(fifo, sendSETL10240MSG10240)
{
	int ans = 0;
	int sock = startSock();
	int exec = 0;
	char *head = "SET";
	unsigned char *msg = "AAAAAAAAAA"; 
	char len[2] = {0};
	char buf[3] = {0};
	boIntToChar(10240, len);
	if(sock == -1) {
		printf("startSock() err%s\n",strerror(errno));
		ans = 0;
	} else {
		if (connect(sock, (struct sockaddr *) &saddr, 
		sizeof(struct sockaddr)) == 0) {
			int exec = send(sock, (void*)head, 3, 0);
			if(exec == -1)	{ printf("send head err\n"); goto error;}
			exec = send(sock, (void*)len, 2, 0 );
			if(exec == -1)	{ printf("send length err\n"); goto error;}
			int i = 0;
			for(; i < 1024; i++) {
				exec = send(sock, (void*)msg, 10, 0 );
				if(exec == -1)	{ printf("send msg err\n"); goto error;}
			}
			
			exec = recv(sock, buf, sizeof(buf), 0);
			if(exec > 0) {
				int p = strstr(buf, "OK");
				if(p) ans = 1;
			} else {printf("send() err[%s]\n", strerror(errno)); goto error;}
		} else {
			printf("connect() err[%s]\n", strerror(errno));
		}
	}
	if(close(sock) == -1) {
		printf("close() err%s\n",strerror(errno));
		ans = 0;
	}
	error:
	if(exec == -1) {
		ans = 0;
	}
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(fifo, send100MSGSET10)
{
	printf("send100MSGSET10() ... \n");
	int ans = 0;
	int exec = 0;
	char *head = "SET";
	unsigned char *msg = "AAAAAAAAAA"; 
	char len[2] = {0};
	char buf[3] = {0};
	int NN = 0;
	int sizeFIFO = 10;
	int r = 0;
	boIntToChar(10, len);
	while (NN < 100) {
		exec = sendMSG(msg, 10, 10);
		if(exec == -1) {
			if(r < sizeFIFO) goto error;
		}
		r++;
		NN++;
	}
	exec = 1;
	ans = 1;
	error:
	if(exec == -1) {
		ans = -1;
	}
	TEST_ASSERT_EQUAL(1, ans);
}
/* ----------------------------------------------------------------------------
 * @brief	Test queue FIFO
 */

TEST(fifo, addOneGetOne)
{
	printf("addOneGetOne() ...\n");
	int flag = -1; 
	int n, i;
	if(bo_initFIFO(2) == 1) {
		char msg[3] = "aaa";
		char buf[10] = {0};
		if(bo_addFIFO(msg, 3) == -1) goto exit;
		if((n = bo_getFIFO(buf, 10)) == -1) goto exit;
		else if(n == 3) {
			for(i = 0; i < 3; i++) {
				if(buf[i] != msg[i]) goto exit;
			}
			flag = 1;
		}
	}
	exit:
	TEST_ASSERT_EQUAL(1, flag);
}

TEST(fifo, fifo2add3)
{
	printf("fifo2add3() ...\n");
	int flag = -1; 
	int n, i;
	if(bo_initFIFO(2) == 1) {
		char msg[3] = "aaa";
		char buf[10] = {0};
		if(bo_addFIFO(msg, 3) == -1) goto exit;
		if(bo_addFIFO(msg, 3) == -1) goto exit;
		if(bo_addFIFO(msg, 3) == -1) {
			for(i=0; i<2; i++) {
				if((n = bo_getFIFO(buf, 10)) == -1) goto exit;
				else if(n == 3) {
					for(i = 0; i < 3; i++) {
						if(buf[i] != msg[i]) goto exit;
					}
				}
			}
			flag = 1;
		} else goto exit;
	}
	exit:
	TEST_ASSERT_EQUAL(1, flag);
}

TEST(fifo, add100get100)
{
	printf("add100get100() ...\n");
	int flag = -1; 
	int n, i, k, ln, j;
	if(bo_initFIFO(7) == 1) {
		char msg[3] = "abc";
		char buf[10] = {0};
		k = 0; ln = 0;
		for(i = 0; i < 100; i++) {
		/*	printf(".");
			if(ln == 10) {ln = 0; printf("\n");} else ln++; */
			if(bo_addFIFO(msg, 3) == -1) {
				if(bo_getFree() > 0) {
					printf("can't add element free = %d\n", 
						bo_getFree());
					goto exit;
				} 
			}
			if(k == 3) {
				if((n = bo_getFIFO(buf, 10)) == -1) {
					printf("can't get element ");
					goto exit;
				} else if(n == 3) {
					for(j = 0; j < 3; j++) {
						if(buf[j] != msg[j]) {
							printf(" buf not equal msg\n");
							goto exit;
						} 
					}
				}
				k = 0;
			} else k++;
		}
		flag = 1;
		bo_delFIFO();
	}
	exit:
	TEST_ASSERT_EQUAL(1, flag);
}

TEST(fifo, getFromEmptyFIFO)
{
	printf("getFromEmptyFIFO() ...\n");
	int flag = -1; 
	int n, i, k, ln, j;
	if(bo_initFIFO(7) == 1) {
		char buf[10];
		if(bo_getFIFO(buf, 10) == -1) flag = 1;
		bo_delFIFO();
	}
	TEST_ASSERT_EQUAL(1, flag);
}

TEST(fifo, getToLittleBuf)
{
	printf("getToLittleBuf() ...\n");
	int flag = -1; 
	int n, i, k, ln, j;
	if(bo_initFIFO(7) == 1) {
		char buf[1];
		char msg[10] = "aaaaaaaaaa";
		if(bo_addFIFO(msg, 10) == 1) {
			if(bo_getFIFO(buf, 1) == -1) flag = 1;
		}
		bo_delFIFO();
	}
	exit:
	TEST_ASSERT_EQUAL(1, flag);
}

TEST(fifo, setNullMsg)
{
	printf("setNullMsg() ...\n");
	int flag = -1; 
	int n, i, k, ln, j;
	if(bo_initFIFO(7) == 1) {
		char buf[1];
		char *msg = NULL;
		if(bo_addFIFO(msg, 10) == -1) {
			flag = 1;
		}
		bo_delFIFO();
	}
	exit:
	TEST_ASSERT_EQUAL(1, flag);
}

TEST(fifo, addBigMsgThanItemFifo)
{
	printf("addBigMsgThanItemFifo() ...\n");
	int flag = -1; 
	int n, i, k, ln, j;
	if(bo_initFIFO(7) == 1) {
		n = BO_FIFO_ITEM_VAL + 10;
		char msg[n];
		for(i = 0; i < n; i++) {
			msg[i] = 'A';
		}
		if(bo_addFIFO(msg, n) == -1) {
			flag = 1;
		}
		bo_delFIFO();
	}
	exit:
	TEST_ASSERT_EQUAL(1, flag);
}

TEST(fifo, testCRC)
{
	printf("testCRC() ... \n");
	char *str = "123456789";
	char str2[20] = " AAAAA AAAAA ";
	unsigned short check = 0x4B37;
	unsigned short width = 16;
	unsigned short poly = 0x8005;
	unsigned short reg_init = 0xFFFF;
	unsigned short xorout = 0x0000;
	int refin = 1;
	int refout = 1;
	gen_tbl_crc16modbus(poly, width, refin);
	
	unsigned short check2 = crc16modbus(str2,
					reg_init, 
					width, 
					refin, 
					refout, 
					xorout);
	printf("check2 = %d\n", check2);
	TEST_ASSERT_EQUAL(check, crc16modbus(str,
					reg_init, 
					width, 
					refin, 
					refout, 
					xorout));
}

