#include <stdio.h>
#include <stdlib.h>
#include "log/bologging.h"
#include "tools/dbgout.h"
#include "nettcp/bo_net.h"


extern void bo_fifo_main();



int main(int argc, char **argv)
{
	dbgout("START -> %s\n", *argv);
	bo_fifo_main();
	dbgout("END\n");
	return 0;
}
