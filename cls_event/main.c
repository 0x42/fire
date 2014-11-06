#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

#include "../src/nettcp/bo_net.h"

int main(int argn, char **argv)
{
	printf("START cls_event server ... \n");
	const int error = -1; 
	int s_serv = -1, s_cl = -1;
	unsigned int port = 8123;
	
	struct sockaddr_in saddr;
	s_serv = socket(AF_INET, SOCK_STREAM, 0);
	if(s_serv == -1) {
		printf("socket() errno[%s]\n", strerror(errno)); 
		goto error;
	}
	memset(&saddr, 0, sizeof(struct sockaddr_in));
	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);
	saddr.sin_addr.s_addr = INADDR_ANY;
	int i = 1;
	setsockopt(s_serv, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	if(bind(s_serv, (struct sockaddr *) &saddr, sizeof(saddr)) == -1) {
		printf("bind() errno[%s]\n", strerror(errno));
		close(s_serv); goto error;
	}
	
	if(listen(s_serv, 5) == -1) {
		printf("listen() errno[%s]\n", strerror(errno));
		close(s_serv);
		goto error;
	}
	
	printf("wait client ..... \n");
	s_cl = accept(s_serv, NULL, NULL);
	if(s_cl == -1) {
		printf("accept() errno[%s]\n", strerror(errno));
		close(s_serv);
		goto error;
	}
	char buf[3] = {0};
	int  n      = 0;
	printf("read msg ...");
	n = recv(s_cl, buf, 3, 0);
	if(n == -1) {
		printf("recv() errno[%s]", strerror(errno));
		close(s_cl);
		close(s_serv);
		goto error;
	} else if(!strstr(buf, "T01")) { 
		printf("get bad msg[%s]", buf);
	}
	printf(".. ok\n");
	
	sleep(30);
	
	printf("send msg ...");
	unsigned char msg[3] = "T01";
	int exec = -1;
	exec = bo_sendAllData(s_cl, msg, 3);
	if(exec == -1) {
		printf("bo_sendAllData() errno[%s]\n", strerror(errno));
		goto error;
	}
	printf(" ..ok[%d]\n", exec);

	
	sleep(60);
	printf("close client sock\n");
	close(s_cl);
	close(s_serv);
	if(error == 1) {
		error:
		printf("ERROR \n");
	}
	printf("END\n");
	return 1;
}