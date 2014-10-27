#include <mcheck.h>
#include <stdio.h>
#include <stdlib.h>
#include "log/bologging.h"
#include "tools/dbgout.h"
#include "nettcp/bo_net.h"
#include "nettcp/bo_fifo.h"

int main(int argc, char **argv)
{
	mtrace();
	dbgout("START -> %s\n", *argv);
	bo_log("TEST");
	dbgout("END\n");
	muntrace();
	return 0;
}