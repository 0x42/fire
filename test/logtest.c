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
	int ans = loggingINFO(msg);
	TEST_ASSERT_EQUAL(1, ans);
}

void loggingINFO_writeOneChar()
{
	char *msg = "a";
	int ans = loggingINFO(msg);
	TEST_ASSERT_EQUAL(1, ans);
}

void loggingERROR_writeNullMsg()
{
	char *msg = NULL;
	int ans = loggingERROR(msg);
	TEST_ASSERT_EQUAL(1, ans);
}

void loggingERROR_writeOneChar()
{
	char *msg = "a";
	int ans = loggingERROR(msg);
	TEST_ASSERT_EQUAL(1, ans);
}
