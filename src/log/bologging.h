#ifndef BOLOGGING_H
#define BOLOGGING_H
/* отключаем static на время тестирования*/
/* #define STATIC static */
/* #define SYSERRFILE "/dev/log" */
#define BO_STATIC 
#define SYSERRFILE "sys.err"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
<<<<<<< HEAD:src/log/logging.h
#include <time.h>
#include <sys/types.h>
=======
>>>>>>> fcb5fcba917ff5bd22527c157ae07b2297664648:src/log/bologging.h
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
//#include "../tools/dbgout.h"
/* отключаем static на время тестирования*/
/* #define STATIC static */


int  bo_log(char *msg, ...);

void bo_setLogParam(char *fname, char *oldfname, int nrow, int maxrow);

void bo_resetLogInit();

int  bo_isBigLogSize(int *nrow, int maxrow, char *name, char *oldname);

void bo_getTimeNow(char *timeStr, int sizeBuf);

#endif
/* [0x42] */