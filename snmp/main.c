#include <stdio.h>
#include <stdlib.h>
#include "../log/bologging.h"
#include "../tools/dbgout.h"

extern void bo_snmp_main();

int main(int argc, char **argv)
{
	dbgout("START\n");
	char *my_ip[] = {"192.168.1.151", "192.168.1.150"};
	int n = 2;
	
	bo_snmp_main(my_ip, n);
	dbgout("END\n");
	return 0;
}

/* 0x42 */
