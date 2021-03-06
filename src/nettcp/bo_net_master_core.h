#ifndef BO_NET_MASTER_CORE_H
#define	BO_NET_MASTER_CORE_H
/*  255 * 30(row_len)=  */

#define BO_MAX_TAB_BUF 8000
#include <stdlib.h>
#include <string.h>
#include "bo_net.h"
#include "../tools/oht.h"
#include "../tools/dbgout.h"
#include "../tools/ocrc.h"
#include "bo_net_master_core_log.h"
#include "bo_cycle_arr.h"

enum m_coreStatus {READHEAD = 0, SET, QUIT, ANSOK, ERR, ADD, READCRC,
                   TAB, READCRC_TAB, READROW, 
                   LOG, SAVELOG,
                   RLO, SENDLOG, SENDNUL, ANS};

struct paramThr {
    int sock;
    TOHT *route_tab;
    /* KA текущ состояние */
    enum m_coreStatus status;
    /* buffer для приема данных SET|LOG|TAB */
    int bufSize;
    unsigned char *buf;
    struct bo_cycle_arr *log;
    /* length длина пакета принятого */
    int length;
};

/* ----------------------------------------------------------------------------
 * @brief       разб сообщения LOG|SET|TAB|RLO
 * @return      тип пришедшего сообщения 
 *              [1]  - SET(измен для таблицы роутов) | TAB
 *              [2]  - LOG(лог ПР) 
 *              [-1] - ERR
 */
int bo_master_core(struct paramThr *p);

/* ----------------------------------------------------------------------------
 * @brief	отправка таблицы одним пакетом
 * @buf         буфер для формирования пакета size(BO_MAX_TAB_BUF) 
 * @return	[-1] - ошибка
 *		[1]  - успешно      
 */
int bo_master_sendTab(int sock, TOHT *tr, char *buf);

/* ----------------------------------------------------------------------------
 * @brief	создание сообщения ROW|LEN|DATA|...ROW|LEN|DATA|CRC из таблицы tr
 * @buf		буфер куда будет записана таблица
 * @return	[0]  - таблица пустая, пакет не сформ
 *		[>0] - строка создана, размер пакета
 *		[-1] - ошибка, все данные не помещ в буфер 
 */
int bo_master_crtPacket(TOHT *tr, char *buf);


#endif	/* BO_NET_MASTER_CORE_H */
/* 0x42 */