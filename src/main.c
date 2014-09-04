#include <stdio.h>
#include "log/logging.h"
#include "log/tools.h"

int main()
{
	printf("START\n");
	loggingINFO("START");
	loggingINFO("END");
	printf("END\n");
	return 0;
}
