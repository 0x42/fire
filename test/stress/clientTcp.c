#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
/*
 * 
 */
static struct sockaddr_in saddr;

int startSock()
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

void boIntToChar(unsigned int x, unsigned char *ans)
{
	unsigned int a = 0;
	a = x >> 8;
	a = a & 0xFF;
	*ans = (char)a;
	a = x & 0xFF;
	*(ans + 1) = (char)a;
}

void t100(int sock)
{
	printf("отправка 1 байта 100 раз");
	char len[2] = {0};
	char *head = "SET";
	char *A = "A";
	int it = 0;
	int msgSize = 100;
	int exec = -1;
	exec = send(sock, (void*)head, 3, 0);
	if(exec == -1) goto error;
	printf("HEAD - ok\n");
	boIntToChar(msgSize, len);
	exec = send(sock, (void*)len, 2, 0);
	if(exec == -1) goto error;
	printf("LEN - ok\n");
	int i = 0;
	for(; it < 100; it++) {
		printf(".");
		i++;
		if(i == 10) {
			printf("\n"); i = 0;
		}
		exec = send(sock, (void*)A, 1, 0);
		if(exec == -1) goto error;
	}
	if(exec == -1) {
		error:
		printf("t100 errno[%s]", strerror(errno));
	}
	printf("BODY - ok\n");
}

void tbigdelay(int sock)
{
	printf("отправка 1 байта 100 раз c задержкой");
	char len[2] = {0};
	char *head = "SET";
	char *A = "A";
	int it = 0;
	int msgSize = 100;
	int exec = -1;
	exec = send(sock, (void*)head, 3, 0);
	if(exec == -1) goto error;
	printf("HEAD - ok\n");
	boIntToChar(msgSize, len);
	exec = send(sock, (void*)len, 2, 0);
	if(exec == -1) goto error;
	printf("LEN - ok\n");
	int i = 0;
	for(; it < 100; it++) {
		printf(".");
		i++;
		if(i == 10) {
			printf(" delay 1 sec \n"); i = 0;
			sleep(10);
		}
		exec = send(sock, (void*)A, 1, 0);
		if(exec == -1) goto error;
	}
	if(exec == -1) {
		error:
		printf("t100 errno[%s]", strerror(errno));
	}
	printf("BODY - ok\n");
}

void talot(int sock)
{
	printf(" длина 10 отправка 2x9 ");
	char len[2] = {0};
	char *head = "SET";
	char *A = "AAAAAAAAA";
	char AB[] = "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB";
	int exec = -1;
	exec = send(sock, (void*)head, 3, 0);
	if(exec == -1) goto error;
	printf("HEAD - ok\n");

	boIntToChar(10, len);
	exec = send(sock, (void*)len, 2, 0);
	if(exec == -1) goto error;
	printf("LEN - ok\n");

	int i = 0;
	int tcp_delay = 1;
	int e = setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &tcp_delay, sizeof(tcp_delay));
	if (e == -1) {
		printf("setsockopt errno[%s]\n", strerror(errno));
		return;
	}
	exec = send(sock, (void*)A, 9, 0);
	if(exec == -1) goto error;
	usleep(10000);
	exec = send(sock, (void*)AB, sizeof(AB), 0);
	if(exec == -1) goto error;
	
	if(exec == -1) {
		error:
		printf("t100 errno[%s]", strerror(errno));
	}
	printf("BODY - ok\n");
}

void t6(int sock)
{
	char *head = "SET";
	unsigned char msg[] = {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF};
	char len[2] = {0};
	int exec = 0;
	exec = send(sock, (void*)head, 3, 0);
	if(exec == -1) goto error;
	printf("HEAD - ok\n");
	boIntToChar(sizeof(msg), len);
	printf("sizeof(msg) = %d \n", sizeof(msg));
	exec = send(sock, (void*)len, 2, 0);
	if(exec == -1) goto error;
	printf("LEN - ok\n");
	exec = send(sock, (void *)msg, sizeof(msg), 0);
	if(exec == -1) {
		error:
		printf("t6 errno[%s]", strerror(errno));
	}
	printf("BODY - ok\n");
}

void get(int sock)
{
	char *head = "GET";
	char h[3] = {0};
	char l[2] = {0};
	char buf[2048] = {0};
	unsigned int length = 0;
	int total = 0, i = 0;
	
	int count = send(sock, head, 3, 0);
	if(count != 3) goto exit;
	
	count = recv(sock, h, 3, 0);
	if(count != 3) goto exit;
	if(strstr(h, "VAL")) {
		printf("VAL -- ok\n");
	
		count = recv(sock, l, 2, 0);
		if(count != 2) goto exit;
		length = boCharToInt(l);
		printf("LENGTH[%d] -- ok\n", length);
		
		
		unsigned char *rcBuf = (unsigned char *)malloc(length);
		if(rcBuf == NULL) goto exit;
		total = 0;
		while(total < length) {
			count = recv(sock, rcBuf+total, length-total, 0);
			if(count < 0) goto exit;
			total += count;
		}
		printf("length =%d --- ---", length);
		for(i = 0; i < length; i++) {
			printf("0x%02x", rcBuf[i]);
		}
		printf("\n");
	}
	exit:
	return;
}

void t2t(int sock)
{
	t100(sock);
	tbigdelay(sock);
}

int main(int argc, char** argv)
{
	printf("START TCP CLIENT TO 127.0.0.1:8888\n");
	int sock = -1;
	int stop = -1;
	char buf[3];
	char H1[1] = "H";
	char H2[3] = "GET";
	char H3[3] = "SET";
	char len[2] = {0};
	int p;
	if((sock = startSock()) != -1) {
		printf("socket created ...\n");
		if (connect(sock, (struct sockaddr *) &saddr, sizeof(struct sockaddr)) == 0) {
			printf("connect to 127.0.0.1 - ok\n");
			int flag = -1;
			while(flag == -1) {
				printf("select msg [H1, T100, TBD, TAL, T6, GET, q]\n");
				scanf("%s", buf);
				p = strstr(buf, "H1");
				if(p) send(sock, (void *) H1, sizeof(H1), 0);
				p = strstr(buf, "T100");
				if(p) t100(sock);
				p = strstr(buf, "TBD");
				if(p) tbigdelay(sock);
				p = strstr(buf, "T2T");
				if(p) t2t(sock);
				p = strstr(buf, "TAL");
				if(p) talot(sock);
				p = strstr(buf, "T6");
				if(p) t6(sock);
				p = strstr(buf, "GET");
				if(p) get(sock);
				p = strstr(buf, "q");
				if(p) {
					stop = 1;
					flag = 1;
				}
			}
		} else {
			printf(strerror(errno));
			printf("can't connect to 127.0.0.1:8888\n repeat?y/n");
			scanf("%s", buf);
			p = strstr(buf, "y");
			if(p) {}
			else {
				stop = 1;
			}
		}
		close(sock);
	}else {
		char *msg = strerror(errno);
		printf("startSock() error[%s]\n", msg);
	}
	printf("END TCP CLIENT\n");
	return(EXIT_SUCCESS);
}

/* 0x42 */