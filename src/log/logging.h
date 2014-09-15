#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include <sys/stat.h>
#include "../tools/dbgout.h"

int logInfo(char *msg);

int logErr(char *msg);

int readNRow(const char *fname);

int writeNRow(FILE *f, char *s, int n);

void setLogParam(char *fname, char *oldfname, int nrow, int maxrow);
#endif
