#include <stdio.h>
#include <stdlib.h>
#include "log/bologging.h"
#include "tools/dbgout.h"
#include "nettcp/bo_net.h"
#include "nettcp/bo_fifo.h"

extern void bo_fifo_main();
/* размер 1,1 Кб */

void prBuf(char *buf, int s)
{
	if(buf != NULL) {
		printf("%s", buf);
	} else printf("NULL");
	printf("\n");
}

int main(int argc, char **argv)
{
	dbgout("START -> %s\n", *argv);
	bo_fifo_main();
	dbgout("END\n");
	return 0;
}
