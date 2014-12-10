#ifndef BO_PARSE_H
#define	BO_PARSE_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bo_asn.h"
#include "../log/bologging.h"

struct PortItem {
    int link;
    int speed;
    int id;
    char descr[21];
};

/* ----------------------------------------------------------------------------
 * @snmp    сообщение полученое от агента
 * @len     длина сообщения
 * @port    полученые данные из сообщения
 * @return  [-1] ERROR [>0] requestId
 */
int bo_parse_oid(unsigned char *snmp, int len, struct PortItem *port);

#endif	/* BO_PARSE_H */

/* 0x42 */