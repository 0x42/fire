#include <stdio.h>
#include <stdlib.h>
#include "../log/bologging.h"
#include "../tools/dbgout.h"

extern void bo_snmp_main();

int main(int argc, char **argv)
{
	dbgout("START\n");
	bo_snmp_main();
	dbgout("END\n");
	return 0;
}

/* 0x42 */
