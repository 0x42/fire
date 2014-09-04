#include "dbgout.h"

void dbgout(char *msg, ...)
{
	/*выставить в -1 если не нужно выводить в stdout*/
	int flgShow = 1; 
	if(flgShow) {
		va_list ap; // указывает на очередной безымян-ый аргумент
		char *p, *sval;
		int ival; double dval;
		va_start(ap, msg); // устанав ap на 1-й безымян-ый аргумент
		for(p = msg; *p; p++) {
			if(*p != '%') {
				putchar(*p);
				continue;
			}
			switch(*++p) {
				case 'd':
					ival = va_arg(ap, int);
					printf("%d", ival);
					break;
				case 'f':
					dval = va_arg(ap, double);
					printf("%f", dval);
					break;
				case 's':
					for(sval = va_arg(ap, char *); *sval; sval++) {
						putchar(*sval);
					}
					break;
				default:
					putchar(*p);
					break;
			}
		}
		va_end(ap); // очистка
	}
}

