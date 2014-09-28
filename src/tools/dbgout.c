#include "dbgout.h"

void dbgout(char *msg, ...)
{
	/*выставить в -1 если не нужно выводить в stdout*/
	int flgShow = 1; 
	/* указывает на очередной безымян-ый аргумент */
	va_list ap; 
	char *p, *sval;
	int ival; 
	double dval;
	if(flgShow) {
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
					sval = va_arg(ap, char *);
					if(sval == NULL) printf("null");
					else  {
						for(; *sval; sval++) {
							putchar(*sval);
						}
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

