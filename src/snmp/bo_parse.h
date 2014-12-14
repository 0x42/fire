#ifndef BO_PARSE_H
#define	BO_PARSE_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bo_asn.h"
#include "../tools/dbgout.h"
#include "../log/bologging.h"
#include "../snmp/bo_snmp.h"

#define PORTITEM_DESCR 21

struct PortItem {
    int flg;    /* [1] OK [-1] don't get data */
    int link;
    int speed;
    int id;
    char descr[PORTITEM_DESCR];
    int descr_len;
};

/* ----------------------------------------------------------------------------
 * @snmp    сообщение полученое от агента
 * @len     длина сообщения
 * @port    полученые данные из сообщения
 * @return  [-1] ERROR [>0] requestId
 */
int bo_parse_oid(unsigned char *snmp, int len, struct PortItem *port);

int bo_parse_INTEGER(unsigned char *buf, int *num);

/* ----------------------------------------------------------------------------
 * @brief	parse OID
 * @value_len	возв размер поля VALUE
 * @return	[-1] ERROR [N BYTE] 
 */
int bo_parse_OID(unsigned char *buf, 
	        int oid[20], 
	        int *oid_len, 
		int *value_len);

/* ----------------------------------------------------------------------------
 * @brief	parse STRING 
 * @return	[-1] ERROR [N BYTE] OK
 */
int bo_parse_STRING(unsigned char *buf,
		    char *str,
		    int *str_len);

struct OID_Next *bo_parse_oid_next();

#endif	/* BO_PARSE_H */

/* 0x42 */