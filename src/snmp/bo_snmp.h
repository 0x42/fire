#ifndef BO_SNMP_H
#define	BO_SNMP_H
#include "bo_asn.h"
#include "../log/bologging.h"
enum {
        SNMP_GET_REQUEST_TYPE      = 0xA0,
        SNMP_GET_NEXT_REQUEST_TYPE = 0xA1,
        SNMP_GET_RESPONSE_TYPE     = 0xA2,
        SNMP_SET_REQUEST_TYPE      = 0xA3
};

int bo_init_snmp();

void bo_snmp_crt_msg(int *oid, int size);

void bo_snmp_crt_next_req(int oid[][14], int n, int m);

unsigned char * bo_snmp_get_msg();

unsigned char * bo_snmp_get_buf();

int bo_snmp_get_msg_len();

void bo_del_snmp();

#endif	/* BO_SNMP_H */
/* 0x42 */