#ifndef BO_NET_MASTER_CORE_LOG_H
#define	BO_NET_MASTER_CORE_LOG_H
#include "../log/bologging.h"
#include "../tools/dbgout.h"
#include "../tools/ocrc.h"
#include "bo_net.h"

enum ka_logStatus {LOGREADHEAD = 0, LOGLOG, LOGREADCRC, LOGOK, LOGERR, 
                   LOGQUIT, LOGNUL};

struct KA_log_param {
    int sock;
    char *buf;
    int bufSize;
    int len;
    /* KA status */
    enum ka_logStatus status;
};

/* ----------------------------------------------------------------------------
 * @brief   принимает LOG|LEN|DATA|CRC
 *	    DATA write in @buf
 *          отправляет OK master'у 
 * @return  [-1] ERROR [>0] - length log [0] - нет лога по такому индексу
 */
int bo_master_core_log(int sock, char *buf, int bufSize);

/*----------------------------------------------------------------------------
 * @brief	отправляет запрос 1) RLO|index => master
 *		принимает лог	  2)          <= LOG
 *		сохраняет LOG в @buf
 * @buf		
 * @index	номер лога 
 * @return	[-1] Error [0] haven't got log [>0] log length
 */
int bo_master_core_logRecv(int sock, int index, char *buf, int bufSize); 

#endif	/* BO_NET_MASTER_CORE_LOG_H */
/* 0x42 */
