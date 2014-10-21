#include <mcheck.h>
#include <stdio.h>
#include <stdlib.h>
#include "../src/tools/dbgout.h"
#include "../src/log/bologging.h"

extern void bo_route_main(int n, char **argv);

int main(int argc, char **argv)
{
	mtrace();
	dbgout("START moxa_route-> %s\n", *argv);
	bo_route_main(argc, argv);
	dbgout("END\n");
	muntrace();
	return 0;
}
