#include <stdio.h>
#include <stdlib.h>
#include "unity/src/unity.h"
#include "../src/log/logging.h"
/*
 * 
 */

//extern int loggingINFO(char *msg);

void loggingINFO_writeNullMsg()
{
	char *msg = NULL;
	loggingINIT();
	int ans = loggingINFO(msg);
	TEST_ASSERT_EQUAL(-1, ans);
}

void loggingINFO_writeOneChar()
{
	char *msg = "a";
	loggingINIT();
	int ans = loggingINFO(msg);
	TEST_ASSERT_EQUAL(1, ans);
}

