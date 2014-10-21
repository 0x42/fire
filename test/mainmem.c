#include <stdio.h>
#include <stdlib.h>
#include "unity/src/unity.h"
#include "../src/tools/bmempool.h"

extern void b_broken_simpletest();
extern void b_pool_test();
extern void b_pool_add100str();
void setUp(void) {}
void tearDown(void) {}

int main()
{
	printf("%s\n", "\n=====\nSTART MEM TEST\n=====\n");
	mtrace();   
//	b_broken_simpletest();
	RUN_TEST(b_pool_test, 12);
	RUN_TEST(b_pool_add100str, 28);
	return 0;
}


/* 0x42 */