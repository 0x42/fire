#ifndef DBGOUT_H
<<<<<<< HEAD
#define DBGOUY_H
#define BO_BDG
=======
#define DBGOUT_H

>>>>>>> fcb5fcba917ff5bd22527c157ae07b2297664648
#include <stdarg.h>
#include <stdio.h>

/* работает как printf. реал-ет след параметры %d %s %f 
   если flgShow == -1 инф не выводит на экран*/
void dbgout(char *msg, ...);

void boIntToChar(unsigned int x, unsigned char *buf);

unsigned int boCharToInt(unsigned char *buf);
#endif
