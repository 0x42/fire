#ifndef BO_NET_MASTER_CORE_H
#define	BO_NET_MASTER_CORE_H
#include <stdlib.h>
#include <string.h>

enum m_coreStatus;

struct paramThr {
    int sock;
    TOHT *route_tab;
    enum m_coreStatus status;
};


void bo_master_core(struct paramThr *p);

#endif	/* BO_NET_MASTER_CORE_H */

/* 0x42 */