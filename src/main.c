#include <stdio.h>
#include "log/logging.h"
#include "tools/dbgout.h"

int main(int argc, char **argv)
{
	dbgout("START -> %s\n", *argv);
	bo_log("%s%s", " INFO ", "START");
	bo_log("%s%d %f %s %", " INFO ", 98, 98.7, "Hi!");
	//writeSysLog("main", "test error", "write error");
	bo_log("%s%s", " INFO ", "END");
	dbgout("END\n");
	return 0;
}
