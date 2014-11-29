#ifndef BO_SNMP_H
#define	BO_SNMP_H
#include "bo_asn.h"

enum {
        SNMP_GET_REQUEST_TYPE = 0xA0,
        SNMP_GET_RESPONSE_TYPE = 0xA2,
        SNMP_SET_REQUEST_TYPE = 0xA3
};

struct SNMP_MSG {
    int ver;
    char *com;
    int pdu_type;
    int request_id;
    int error;
    int error_i;
};

#endif	/* BO_SNMP_H */
/* 0x42 */