#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../log/bologging.h"
#include "../tools/dbgout.h"
#include "bo_asn.h"
#include "bo_snmp.h"

void bo_snmp_main()
{
	int exec = -1;
	dbgout("bo_main_snmp ... run\n");
	char *oid = "1.3.6.1.2.1.1.3.0";
	
	exec = bo_init_snmp();
	if(exec == -1) bo_log("bo_snmp_main() ERROR can't create data for run snmp");
	
	bo_snmp_crt_msg(oid, strlen(oid));
	
	bo_del_snmp();
	dbgout("\nbo_main_snmp ... end\n");
}