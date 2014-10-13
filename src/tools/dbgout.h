#ifndef DBGOUT_H
#define DBGOUT_H

#include <stdarg.h>
#include <stdio.h>

/* работает как printf. реал-ет след параметры %d %s %f 
   если flgShow == -1 инф не выводит на экран*/
void dbgout(char *msg, ...);

void boIntToChar(unsigned int x, unsigned char *buf);

unsigned int boCharToInt(unsigned char *buf);
#endif



