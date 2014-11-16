#include  "unity_fixture.h"

TEST_GROUP_RUNNER(route)
{
	RUN_TEST_CASE(route, simpleTest);
	RUN_TEST_CASE(route, sendgetTest);
	RUN_TEST_CASE(route, boMasterCoreTest);
//	RUN_TEST_CASE(route, sendNULLTest);
}
