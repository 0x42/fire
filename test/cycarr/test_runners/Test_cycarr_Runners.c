#include "unity_fixture.h"

TEST_GROUP_RUNNER(cycarr)
{
	RUN_TEST_CASE(cycarr, addGet);
	RUN_TEST_CASE(cycarr, addGet2);
	RUN_TEST_CASE(cycarr, addTen);
	RUN_TEST_CASE(cycarr, add100getLast);
}

