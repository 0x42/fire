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

#define BO_OPT_SW_PORT_N 5

struct OPT_SWITCH {
        char ip[16];
        struct PortItem ports[BO_OPT_SW_PORT_N];
        int port_n;
};

void bo_snmp_main(char *ip, int n);


#endif	/* BO_SNMP_MNG_H */

/* 0x42 */