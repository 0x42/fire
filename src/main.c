#include <stdio.h>
#include "log/logging.h"
#include "log/tools.h"

int main()
{
	
	printf("START\n");
	loggingINIT();
	loggingINFO("START");
	
	loggingINFO("END");
	loggingFIN();
	printf("END\n");
	return 0;
}
