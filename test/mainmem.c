#include <stdio.h>
#include <stdlib.h>
#include "unity/src/unity.h"
#include "../src/tools/bmempool.h"

extern void b_broken_simpletest();

int main()
{
	printf("%s\n", "\n=====\nSTART MEM TEST\n=====\n");
	mtrace();   
	b_broken_simpletest();
	b_pool_test();
	return 0;
}


/* 0x42 */