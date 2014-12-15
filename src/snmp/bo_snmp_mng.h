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

/* ----------------------------------------------------------------------------
 * @brief   опрос оптич-ких свитчей.
 * @ip      массив ip switch'ей которые будем опрашивать
 * @n       кол-во опраш устройств
 */
void bo_snmp_main(char *ip, int n);

void bo_snmp_lock_mut();

void bo_snmp_unlock_mut();

/* ----------------------------------------------------------------------------
 * @brief   возвращает таблицу состояния магистрали. Перед чтением данных
 *          вызываем bo_snmp_lock_mut/bo_snmp_unlock_mut 
 * @return  OPT_SWITCH[] / NULL
 */
struct OPT_SWITCH *bo_snmp_get_tab();


#endif	/* BO_SNMP_MNG_H */

/* 0x42 */