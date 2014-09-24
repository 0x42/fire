#include <stdio.h>
#include "log/logging.h"
#include "tools/dbgout.h"
#include "nettcp/bo_net.h"

int main(int argc, char **argv)
{
	dbgout("START -> %s\n", *argv);
	bo_log("%s%s", " INFO ", "START");
	int sock = initServerSock();
	int client_sock = 0;
	char buffer[256] = {0};
	int nbytes = 0;
	if(sock != -1) {
		dbgout("listen socket ...\n");
		if(waitConnect(sock, &client_sock) != -1) {
			nbytes = read(client_sock, buffer, 256);
			printf("get data[%s]", buffer);
			dbgout("close client socket");
			close(client_sock);
		} else {
			bo_log("waitConnect() error");
		}
		dbgout("close server socket");
		close(sock);		
	} else {
		bo_log("main cannot create socket");
	}
	//writeSysLog("main", "test error", "write error");
	bo_log("%s%s", " INFO ", "END");
	dbgout("END\n");
	return 0;
}
