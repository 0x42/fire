#ifndef BO_NET_MASTER_CORE_H
#define	BO_NET_MASTER_CORE_H
/*  255row*30(row_len)=  */
#define BO_MAX_TAB_BUF 8000
#include <stdlib.h>
#include <string.h>
#include "bo_net.h"
#include "../tools/oht.h"
#include "../tools/dbgout.h"
#include "../tools/ocrc.h"

enum m_coreStatus {READHEAD = 0, SET, QUIT, ANSOK, ERR, ADD, READCRC, LOG,
                   TAB, READCRC_TAB, READROW};

struct paramThr {
    int sock;
    TOHT *route_tab;
    /* KA текущ состояние */
    enum m_coreStatus status;
    /* buffer для приема данных */
    int bufSize;
    unsigned char *buf;
    /* length длина пакета принятого */
    int length;
};

/* ----------------------------------------------------------------------------
 * @return      тип пришедшего сообщения 
 *              [1]  - SET(измен для таблицы роутов) 
 *              [2]  - LOG(лог ПР) 
 *              [-1] - ERR
 */
int bo_master_core(struct paramThr *p);

#endif	/* BO_NET_MASTER_CORE_H */

/* 0x42 */