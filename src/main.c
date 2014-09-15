#include <stdio.h>
#include "log/logging.h"
#include "tools/dbgout.h"

int main()
{
	dbgout("START\n");
	logInfo("START");
	logInfo("END");
	dbgout("END\n");
	return 0;
}
