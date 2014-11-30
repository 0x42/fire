#ifndef BO_SNMP_H
#define	BO_SNMP_H
#include "bo_asn.h"
#include "../log/bologging.h"
enum {
        SNMP_GET_REQUEST_TYPE = 0xA0,
        SNMP_GET_RESPONSE_TYPE = 0xA2,
        SNMP_SET_REQUEST_TYPE = 0xA3
};

int bo_init_snmp();

void bo_snmp_crt_msg(char *oid, int size);

void bo_del_snmp();
#endif	/* BO_SNMP_H */
/* 0x42 */