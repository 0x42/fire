#include <stdio.h>
#include <stdlib.h>
#include "log/bologging.h"
#include "tools/dbgout.h"
#include "nettcp/bo_net.h"

extern void bo_fifo_main();

void test(char	**txt) 
{
	static char *s = "kura bjaka"; 
	*txt = s;
}

int main(int argc, char **argv)
{
	dbgout("START -> %s\n", *argv);
	char *txt = NULL;
	test(&txt);
	printf("%s\n",txt);
	bo_fifo_main();
	dbgout("END\n");
	return 0;
}
