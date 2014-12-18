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

struct OID_Next {	
	int  link[20];
	int speed[20];
	int descr[20];
	
	int link_size;
	int speed_size;
	int descr_size;
};

int bo_init_snmp();

void bo_snmp_crt_msg(int *oid, int size);

void bo_snmp_crt_next_req(int oid[][14], int n, int m);

void bo_snmp_crt_next_req2(struct OID_Next *oid_next);

unsigned char * bo_snmp_get_msg();

unsigned char * bo_snmp_get_buf();

int bo_snmp_get_msg_len();

int bo_snmp_get_buf_len();


void bo_del_snmp();

#endif	/* BO_SNMP_H */
/* 0x42 */