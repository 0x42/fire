#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include "../../src/nettcp/bo_net.h"
#include "../../src/nettcp/bo_fifo.h"
#include "../../src/tools/ocrc.h"

#include "unity_fixture.h"

TEST_GROUP(fifo);

TEST_SETUP(fifo) {}

TEST_TEAR_DOWN(fifo) {}

void *testThr(void *arg);
void *get10thr(void *arg);
void *startServer(void *arg);

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

TEST(fifo, sendOneByte) /* NEED RUN SERVER*/
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
			int exec = bo_sendAllData(sock, msg, 1);
			if(exec != -1)	{
				exec = bo_recvAllData(sock, buf, sizeof(buf), 3);
				if(exec > 0) {
					char *p = strstr(buf, "ERR");
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

TEST(fifo, getFromEmptyFifo) /* NEED RUN SERVER*/
{
	int exec = -1;
	char buf[20] = {0};
	int ans = -1;
	
	printf("getFromEmptyFifo() ... \n");
	exec = bo_recvDataFIFO("127.0.0.1", 8888, buf, 20);
	if(exec == -1) {
		printf("recv error %s\n", strerror(errno)); 
	} else {
		if(exec == 0) {
			ans = 1;
		} else {
			printf("length[%d] wait length 0 buf[%s]\n", exec, buf);
		}
	}
	TEST_ASSERT_EQUAL(1, ans);
}
/* ----------------------------------------------------------------------------
 * @brief	отправляем сообщение SET length=10 msg size = 10
 *		-102 = OK
 *		-101 = NO
 *		-100 = ERR
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
				if(strstr(buf, "OK")) ans = -102;
				if(strstr(buf, "ERR")) ans = -100;
				if(strstr(buf, "NO")) ans = -101;
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
			struct timeval tval;
			tval.tv_sec = 0;
			tval.tv_usec = 100000;
			/* устан максимальное время ожидания одного пакета */
			setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(tval));
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
				bo_recvAllData(sock, buf, length, length);
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

TEST(fifo, sendSETL10MSG10) /* NEED RUN SERVER*/
{
	printf("sendSETL10MSG10() ... \n");
	int ans = 0;
	int i = 0;
	int exec = 0;
	int sock;
	int bufSize = 0;
	int msgSize = 10;
	unsigned char *msg = "AAAAAAAAAA";
	unsigned char buf[1200] = {0};
	ans = bo_sendDataFIFO("127.0.0.1", 8888, msg, msgSize);
	if(ans != 1) goto error;
	exec = bo_recvDataFIFO("127.0.0.1", 8888, buf, 1200);
	if(exec > 0) {
		if(msgSize == bufSize) {
			ans = -1;
			for(i = 0; i < msgSize; i++) {
				if(msg[i] != buf[i]) goto error;
			}
			ans = 1;
		}
		exec = bo_recvDataFIFO("127.0.0.1", 8888, buf, 1200);
		if(exec == 0 ) {
			ans = 1;
		} else {
			printf("FIFO is not empty\n");
			ans = -1;
		}
	} else if(exec == 0){
		printf("FIFO is empty \n");
		ans = -1;
	} else {
		printf("error when recv data from FIFO \n");
		ans = -1;
	}
	error:
	TEST_ASSERT_EQUAL(1, ans);
}

/* ----------------------------------------------------------------------------
 * @brief	отправляем сообщение SET length=10 msg size = 9
 */
TEST(fifo, sendSETL10MSG9) /* NEED RUN SERVER */
{
	printf("sendSETL10MSG9() ... \n");
	int ans = 0;
	int exec = 1;
	int sock;
	unsigned char msg[10] = "AAAAAAAAA";
	unsigned char *buf;
	int bufSize = 0;
	int msgSize = 9;
	int length = 10;
	ans = sendMSG(msg, msgSize, length);
	TEST_ASSERT_EQUAL(-100, ans);
}

TEST(fifo, sendOnlyHead) /* NEED RUN SERVER */
{
	printf("sendOnlyHead() ... \n");
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
				char *p = strstr(buf, "ERR");
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

TEST(fifo, send100MSGSET10) /* NEED RUN SERVER */
{
	printf("send100MSGSET10() ... \n");
	int ans = 0;
	int exec = 0;
	int flag = -1;
	int i_rand;
	int Nsize = 20;
	unsigned char msg[20] = {0};
	unsigned char id[8] = {0};
	unsigned char packet[28] = {0};
	char len[2] = {0};
	int NN = 0;
	int R = 0;
	int i = 0;
	while (NN < 20) {
		i++;
		i_rand = i;
		sprintf(id, "%08d", i_rand);
		memcpy(packet, id, 8);
		sprintf(msg, "%20d", i);
		memcpy( (packet + 8) , msg, 20);
		
		exec = bo_sendDataFIFO("127.0.0.1", 8888, packet, 28);
		if(exec == -1) { printf("testThr send %s\n", strerror(errno)); goto error; }
		
		/*
		memset(buf, 0, Nsize);
		exec = bo_recvDataFIFO("127.0.0.1", 8888, buf, Nsize);
		if(exec == -1) {
			printf("recv error %s\n", strerror(errno)); 
			goto error;
		}
		
		for(i = 0; i < Nsize; i++) {
			if(msg[i] != buf[i]) {
				printf("msg[%s]!=buf[%s]\n", msg, buf );
				goto error;
			}
		}
		*/
		NN++;
	}
	exec = 1;
	ans = 1;
	
	if(exec == -1) {
		error:
		ans = -1;
	}
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(fifo, testThrModel) 
{
	int ans = -1, exec = -1, err = 1; 
	pthread_t thr1, thr2;
	printf("testThrModel ... run \n");
	bo_setLogParam("thr.log", "thr.log_old", 0, 1000);
	exec = pthread_create(&thr1, NULL, &startServer, NULL);
	if(exec == -1) { printf("create thr1 startServer ... "); goto error;}
	
	exec = pthread_create(&thr2, NULL, &testThr, NULL);
	if(exec == -1) { printf("create thr2 testThr ... "); goto error;}
	
	pthread_join(thr2, NULL);
	printf("... test finish\n");
	ans = 1;
	if(err == -1) {
		error: printf("ERROR\n"); ans = -1;
	}
	TEST_ASSERT_EQUAL(1, ans);
}

void *get10thr(void *arg)
{
	unsigned char msg[21] = {0};
	unsigned char buf[30] = {0};
	int i = 0, exec = -1, j = 0;
	const int err = -1;
	while(i < 2000) {
		exec = bo_getFifoVal(buf, 30);
		if(exec != -1) {
			sprintf(msg, "%20d", i);
//			printf("[%d]get10thr[%d][%s]\n", i, exec, buf);
			
			if(exec != 20) {
				bo_log("get10thr() return length value from FIFO");
				goto error;
			}
			for(j = 0; j < exec; j++) {
				if(buf[j]!= msg[j]) {
					bo_log("get10thr() return bad value from FIFO");
					bo_log("value[%s] msg[%s]", buf, msg);
					goto error;
				}
			}
			i++;
		} else {
//			bo_log("get10thr fifo no data");
		}
	}
	if(err == 1) {
		error:
		dbgout("!!! ERROR !!!");
		bo_log(" ERROR ");
	}
}

void *testThr(void *arg)
{
	int exec = -1, err = -1;
	printf("testThr ... \n");
	int Nsize = 20, i = 0;
	unsigned char msg[20] = {0};
	unsigned char id[8] = {0};
	unsigned char packet[28] = {0};
	pthread_t thr3; int st_thr = -1;
	int i_rand = 0;
	srand( (unsigned) time(NULL));
	
	int n = 0;
	for(i = 0; i < 2000; i++) {
		n++;
		if(n == 999) {sleep(5); n = 0;}
//		i_rand = rand()%1000;
		i_rand = i;
		sprintf(id, "%08d", i_rand);
		memcpy(packet, id, 8);
		sprintf(msg, "%20d", i);
		memcpy( (packet + 8) , msg, 20);
		
		exec = bo_sendDataFIFO("127.0.0.1", 8888, packet, 28);
		if(exec == -1) { printf("testThr send %s\n", strerror(errno)); goto error; }
		
		if(st_thr == -1) {
			exec = pthread_create(&thr3, NULL, &get10thr, NULL);
			if(exec == -1) { printf("create thr3 get10thr ... "); goto error;}
			st_thr = 1;
		}
		
	}
	bo_log("TEST THR <====== END ");
	pthread_join(thr3, NULL);
	if(err == 1) {
		error: printf("ERROR");
	}
}

extern void bo_fifo_thrmode(int port, int queue_len, int fifo_len);
void *startServer(void *arg)
{
	printf("startServer ...\n");
	bo_fifo_thrmode(8888, 10, 1000);
}

/* TEST send id msg 1,2,1,2,1,2 */
void *getThrIDmsg(void *arg);
void *testThrIDmsg(void *arg);

TEST(fifo, testIDmsg)
{
	int ans = -1, exec = -1, err = 1; 
	pthread_t thr1, thr2;
	printf("testIDmsg ... run \n");
	bo_setLogParam("thr.log", "thr.log_old", 0, 1000);
	exec = pthread_create(&thr1, NULL, &startServer, NULL);
	if(exec == -1) { printf("create thr1 startServer ... "); goto error;}
	
	exec = pthread_create(&thr2, NULL, &testThrIDmsg, NULL);
	if(exec == -1) { printf("create thr2 testThrIDmsg ... "); goto error;}
	
	pthread_join(thr2, NULL);
	printf("... test finish\n");
	ans = 1;
	if(err == -1) {
		error: printf("ERROR\n"); ans = -1;
	}
	TEST_ASSERT_EQUAL(1, ans);
}

void *getThrIDmsg(void *arg)
{
	unsigned char msg[21] = {0};
	unsigned char buf[30] = {0};
	int i = 0, exec = -1, j = 0;
	const int err = -1;
	while(i < 10) {
		exec = bo_getFifoVal(buf, 30);
		if(exec != -1) {
			sprintf(msg, "%20d", i);
			printf("[%d]getThrIDmsg[%d][%s]\n", i, exec, buf);
			
			if(exec != 20) {
				bo_log("get10thr() return length value from FIFO");
				goto error;
			}
			for(j = 0; j < exec; j++) {
				if(buf[j]!= msg[j]) {
					bo_log("get10thr() return bad value from FIFO");
					bo_log("value[%s] msg[%s]", buf, msg);
					goto error;
				}
			}
			i++;
		} else {
//			bo_log("get10thr fifo no data");
		}
	}
	if(err == 1) {
		error:
		bo_log(" ERROR ");
	}
}

void *testThrIDmsg(void *arg)
{
	int exec = -1, err = -1;
	printf("testThr ... \n");
	int Nsize = 20, i = 0;
	unsigned char msg[20] = {0};
	unsigned char id[8] = {0};
	unsigned char packet[28] = {0};
	pthread_t thr3; int st_thr = -1;
	int i_rand = 0;
	srand( (unsigned) time(NULL));
	int ddd = 1;
	int n = 0;
	int j = 0;
	for(i = 0; i < 10; i++) {
		
		sprintf(id, "%08d", ddd);
		memcpy(packet, id, 8);
		sprintf(msg, "%20d", i);
		memcpy( (packet + 8) , msg, 20);
		
		if(ddd == 1) ddd = 2;
		else ddd = 1;
		
		/*
		printf("send to FIFO [");
		for(j=0; j<28; j++) { printf("%c", packet[j]);} printf("]\n");
		*/
		
		exec = bo_sendDataFIFO("127.0.0.1", 8888, packet, 28);
		if(exec == -1) { printf("testThr send %s\n", strerror(errno)); goto error; }
		
		if(st_thr == -1) {
			exec = pthread_create(&thr3, NULL, &getThrIDmsg, NULL);
			if(exec == -1) { printf("create thr3 getThrIDmsg ... "); goto error;}
			st_thr = 1;
		}
		
	}
	bo_log("TEST THR <====== END ");
	pthread_join(thr3, NULL);
	if(err == 1) {
		error: printf("ERROR");
	}
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
		char msg[10] = "abcdefighj";
		char buf[10] = {0};
		if(bo_addFIFO(msg, 10) == -1) goto exit;
		if((n = bo_getFIFO(buf, 10)) == -1) goto exit;
		else if(n == 10) {
			for(i = 0; i < 10; i++) {
				if(buf[i] != msg[i]) {
					printf("buf[%s]!=msg[%s]\n", buf, msg);
					goto exit;
				}
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
		char msg[3] = "abc";
		char buf[10] = {0};
		if(bo_addFIFO(msg, 3) == -1) { printf("can't add value 1\n"); goto exit;}
//		bo_printFIFO();
		if(bo_addFIFO(msg, 3) == -1) { printf("can't add value 2\n"); goto exit;}
		if(bo_addFIFO(msg, 3) != -1) {
			for(i = 0; i < 2; i++) {
				if((n = bo_getFIFO(buf, 10)) == -1) goto exit;
				else if(n == 3) {
					for(i = 0; i < 3; i++) {
						if(buf[i] != msg[i]) {
							printf("recv bad value from fifo\n");
							goto exit;
						}
					}
				}
			}
			flag = 1;
		} else { printf("can't add value 3 \n"); goto exit;}
	} else printf("can't init fifo\n");
	exit:
	TEST_ASSERT_EQUAL(1, flag);
}

TEST(fifo, add100get100)
{
	printf("add100get100() ...\n");
	int flag = -1; 
	int n, i, k, ln, j;
	if(bo_initFIFO(7) == 1) {
		char msg[] = "abc";
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

TEST(fifo, addget100)
{
	printf("addget100() ...\n");
	int flag = -1; 
	int n, i, k, ln, j;
	if(bo_initFIFO(100) == 1) {
		char msg[10] = "abcdefikjh";
		char buf[10] = {0};
		k = 0; ln = 0;
		for(i = 0; i < 100; i++) {
			if(bo_addFIFO(msg, 10) == -1) {
				if(bo_getFree() > 0) {
					printf("can't add element free = %d\n", 
						bo_getFree());
					goto exit;
				} 
			}
				if((n = bo_getFIFO(buf, 10)) == -1) {
					printf("can't get element ");
					goto exit;
				} else if(n == 10) {
					for(j = 0; j < 10; j++) {
						if(buf[j] != msg[j]) {
							printf(" buf[%s] != msg[%s]\n", buf, msg);
							goto exit;
						} 
					}
				}
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

/* 0x42 */