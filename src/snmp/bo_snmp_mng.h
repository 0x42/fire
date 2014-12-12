#ifndef BO_SNMP_MNG_H
#define	BO_SNMP_MNG_H
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "../log/bologging.h"
#include "../tools/dbgout.h"
#include "bo_asn.h"
#include "bo_snmp.h"
#include "../netudp/bo_udp.h"
#include "../snmp/bo_parse.h"

struct OPT_SWITCH {
        char ip[15];
        struct PortItem ports[5];
        int port_n;
        
};

void bo_snmp_main(char *ip, int n);



#endif	/* BO_SNMP_MNG_H */

/* 0x42 */