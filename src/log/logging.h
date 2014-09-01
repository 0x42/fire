#ifndef LOGGING_H
#define LOGGING_H
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include "tools.h"

void loggingINIT();

void loggingINFO(char *msg);

void loggingFIN();

#endif
