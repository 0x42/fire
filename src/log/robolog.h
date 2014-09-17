/* из докум доступно flash 4M - 256K
 * /etc /home /tmp /usr/bin доступные каталоги
 * RAM монтир как /var/
 */
#ifndef ROBOLOG_H
#define	ROBOLOG_H
#include <stdio.h>
#include "logging.h"

void bo_robLogInit(char *fname, char *foldname, int row, int maxrow);

int bo_robLog(char *msg);

#endif	/* ROBOLOG_H */

/* [0x42] */