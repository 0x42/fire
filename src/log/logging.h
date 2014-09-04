#ifndef LOGGING_H
#define LOGGING_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <syslog.h>
#include <time.h>
#include "tools.h"

int loggingINFO(char *msg);

int loggingERROR(char *msg);

#endif
