#include <stdio.h>
#include "log/logging.h"
#include "tools/dbgout.h"

int main()
{
	dbgout("START\n");
	loggingINFO("START");
	loggingINFO("END");
	dbgout("END\n");
	return 0;
}
