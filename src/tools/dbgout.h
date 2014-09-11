#ifndef DBGOUT_H
#define DBGOUY_H

#include <stdarg.h>
#include <stdio.h>

/* работает как printf. реал-ет след параметры %d %s %f 
   если flgShow == -1 инф не выводит на экран*/
void dbgout(char *msg, ...);

#endif
