#include <stdio.h>
#include "log/bologging.h"
#include "tools/dbgout.h"
#include "nettcp/bo_net.h"

//extern int bo_initServerSock();
void start();

int main(int argc, char **argv)
{
	dbgout("START -> %s\n", *argv);
	bo_log("%s%s", " INFO ", "START");
	start();
	bo_log("%s%s", " INFO ", "END");
	dbgout("END\n");
	return 0;
}

void start()
{
	int sock; 
	int client_sock = 0;
	char buffer[256] = {0};
	int n = 0;
	sock = bo_initServerSock();
	if(sock != -1) {
		dbgout("listen socket ...\n");
		bo_listen(sock, 20);
		while(n != 5) {
			n++;
			if(bo_waitConnect(sock, &client_sock) != -1) {
				read(client_sock, buffer, 256);
				printf("get data[%s]", buffer);
				dbgout("close client socket");
				close(client_sock);
			} else {
				bo_log("waitConnect() error");
			}
		}
		dbgout("close server socket");
		close(sock);		
	} else {
		bo_log("main cannot create socket");
	}
}