#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include "unity_fixture.h"

TEST_GROUP(fifo);

TEST_SETUP(fifo)
{}

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


TEST(fifo, sendOneByte)
{
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
				ans = 1;
//				exec = recv(sock, buf, sizeof(buf), 0);
//				if(exec > 0) {
//					int p = strstr(buf, "ERR");
//					if(p) ans = 1;
//				} else {
//					printf("not get ERR");
//				}
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
TEST(fifo, sendSETL10MSG10)
{
	int ans = 0;
	int sock = startSock();
	int exec = 0;
	char *head = "SET";
	unsigned char *msg = "AAAAAAAAAA"; 
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
			exec = send(sock, (void*)len, 2, 0 );
			if(exec == -1)	{ printf("send length err\n"); goto error;}
			exec = send(sock, (void*)msg, 10, 0 );
			if(exec == -1)	{ printf("send msg err\n"); goto error;}
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
/* ----------------------------------------------------------------------------
 * @brief	отправляем сообщение SET length=10 msg size = 9
 */
TEST(fifo, sendSETL10MSG9)
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
			exec = send(sock, (void*)len, 2, 0 );
			if(exec == -1)	{ printf("send length err\n"); goto error;}
			exec = send(sock, (void*)msg, 9, 0 );
			if(exec == -1)	{ printf("send msg err\n"); goto error;}
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
/* ----------------------------------------------------------------------------
 * @brief	отправляем сообщение SET length=10 msg size = 23
 */
TEST(fifo, sendSETL10MSG23)
{
	int ans = 0;
	int sock = startSock();
	int exec = 0, p = 0;
	int tcp_delay = 1;
	char *head = "SET";
	unsigned char *msg = "AAAAA AAAAA AAAAA AAAAA"; 
	char len[2] = {0};
	char buf[3] = {0};
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
			exec = recv(sock, buf, sizeof(buf), 0);
			if(exec > 0) {
				p = strstr(buf, "OK");
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

TEST(fifo, send100MSGSET1024)
{
	int ans = 0;
	int exec = 0;
	char *head = "SET";
	unsigned char *msg = "AAAAAAAAAA"; 
	char len[2] = {0};
	char buf[3] = {0};
	int NN = 0;
	boIntToChar(10240, len);
	while (NN < 100) {
		int sock = startSock();
		if(sock == -1) {
			printf("startSock() err%s\n",strerror(errno));
			ans = 0; goto error;
		} 
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
		if(close(sock) == -1) {
			printf("close() err%s\n",strerror(errno));
			ans = 0;
		}
		NN++;
	}
	error:
	if(exec == -1) {
		ans = 0;
	}
	TEST_ASSERT_EQUAL(1, ans);
}