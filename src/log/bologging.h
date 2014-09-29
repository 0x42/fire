#ifndef __BOLOGGING_H
#define __BOLOGGING_H
/*
#define STATIC static
#define SYSERRFILE "/dev/log"
 */
#define STATIC 
#define SYSERRFILE "sys.err"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
//#include "../tools/dbgout.h"
/* отключаем static на время тестирования*/
/* #define STATIC static */


int bo_log(char *msg, ...);

void bo_setLogParam(char *fname, char *oldfname, int nrow, int maxrow);

void bo_resetLogInit();

int bo_isBigLogSize(int *nrow, int maxrow, char *name, char *oldname);

void bo_getTimeNow(char *timeStr, int sizeBuf);

#endif
/* [0x42] */