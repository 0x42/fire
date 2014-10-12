#include <stdio.h>
#include <stdlib.h>
#include "log/bologging.h"
#include "tools/dbgout.h"
//#include "nettcp/bo_net.h"


extern void bo_fifo_main();

int main(int argc, char **argv)
{
	dbgout("START -> %s\n", *argv);
<<<<<<< HEAD
	bo_log("%s%s", " INFO ", "START");
//	int sock = initServerSock();
//	int client_sock = 0;
//	char buffer[256] = {0};
//	int nbytes = 0;
//	if(sock != -1) {
//		dbgout("listen socket ...\n");
//		if(waitConnect(sock, &client_sock) != -1) {
//			nbytes = read(client_sock, buffer, 256);
//			printf("get data[%s]", buffer);
//			dbgout("close client socket");
//			close(client_sock);
//		} else {
//			bo_log("waitConnect() error");
//		}
//		dbgout("close server socket");
//		close(sock);		
//	} else {
//		bo_log("main cannot create socket");
//	}
	//writeSysLog("main", "test error", "write error");
	bo_log("%s%s", " INFO ", "END");
=======
	bo_fifo_main();
>>>>>>> fcb5fcba917ff5bd22527c157ae07b2297664648
	dbgout("END\n");
	return 0;
}
