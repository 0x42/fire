#ifndef BOLOGGING_H
#define BOLOGGING_H
/* отключаем static на время тестирования*/
/* #define STATIC static */
/* #define SYSERRFILE "/dev/log" */
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
#include <pthread.h>
//#include "../tools/dbgout.h"
/* отключаем static на время тестирования*/
/* #define STATIC static */

/*THR SAFE*/
int  bo_log(char *msg, ...);

/* THR SAFE*/
void bo_setLogParam(char *fname, char *oldfname, int nrow, int maxrow);
/* NO THR SAFE*/
void bo_resetLogInit();
/* NO THR SAFE*/
int  bo_isBigLogSize(int *nrow, int maxrow, char *name, char *oldname);
/* NO THR SAFE*/
void bo_getTimeNow(char *timeStr, int sizeBuf);

#endif
/* [0x42] */