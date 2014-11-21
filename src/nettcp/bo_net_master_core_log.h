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

/* read Log from sock */
int bo_master_core_log(int sock, char *buf, int bufSize);

/* send RLO|Index return LOG*/
int bo_master_core_logRecv(int sock, int index, char *buf, int bufSize);

#endif	/* BO_NET_MASTER_CORE_LOG_H */
/* 0x42 */
