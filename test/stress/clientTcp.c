#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
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

int main(int argc, char** argv)
{
	printf("START TCP CLIENT TO 127.0.0.1:8888\n");
	int sock = -1;
	int stop = -1;
	char buf[3];
	char H1[1] = "H";
	char H2[3] = "GET";
	char H3[3] = "SET";
	int p;
	if((sock = startSock()) != -1) {
		printf("socket created ...\n");
		while(stop == -1) {
			if (connect(sock, (struct sockaddr *) &saddr, sizeof(struct sockaddr)) == 0) {
				printf("connect to 127.0.0.1 - ok\n");
				int flag = -1;
				while(flag == -1) {
					printf("select msg [H1, GET, SET, q]\n");
					scanf("%s", buf);
					p = strstr(buf, "H1");
					if(p) send(sock, (void *) H1, sizeof(H1), 0);
					else {
						p = strstr(buf, "GET");
						if(p) send(sock, (void*)H2, sizeof(H2), 0);
						else {
							p = strstr(buf, "SET");
							if(p) send(sock, (void*)H3, sizeof(H3), 0);
							else {
								p = strstr(buf, "q");
								if(p) {
									stop = 1;
									flag = 1;
								}
							}
						}
						
						
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