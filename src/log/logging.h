#ifndef LOGGING_H
#define LOGGING_H
/* отключаем static на время тестирования*/
//#define STATIC static
//#define SYSERRFILE "/dev/log"
#define STATIC 
#define SYSERRFILE "sys.err"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "../tools/dbgout.h"

int bo_log(char *msg, ...);

void bo_resetLogInit();

int bo_isBigLogSize(int *nrow, int maxrow, char *name, char *oldname);

void bo_getTimeNow(char *buf, int size);

#endif
/* [0x42] */