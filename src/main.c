#include <mcheck.h>
#include <stdio.h>
#include <stdlib.h>
#include "log/bologging.h"
#include "tools/dbgout.h"
#include "nettcp/bo_net.h"
#include "nettcp/bo_fifo.h"

extern void bo_fifo_main(int n, char **argv);
/* размер 1,1 Кб */

int main(int argc, char **argv)
{
	mtrace();
	dbgout("START -> %s\n", *argv);
	bo_log("TEST");
	bo_fifo_main(argc, argv);
	dbgout("END\n");
	muntrace();
	return 0;
}