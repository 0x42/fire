#include  "unity_fixture.h"

TEST_GROUP_RUNNER(route)
{
//	RUN_TEST_CASE(route, simpleTest);/* NEED RUN SERVER*/
	//RUN_TEST_CASE(route, sendgetTest);/* NEED RUN SERVER*/
//	RUN_TEST_CASE(route, boMasterCoreTest);
//	RUN_TEST_CASE(route, boMasterCoreTabTest);
//	RUN_TEST_CASE(route, boMasterCoreBadTabTest);
//	RUN_TEST_CASE(route, sendNULLTest); /* NEED RUN SERVER */
	RUN_TEST_CASE(route, sendLogTest); /* NEED RUN SERVER */
	RUN_TEST_CASE(route, sendGetLogTest); /* NEED RUN SERVER */
	RUN_TEST_CASE(route, getNullLogTest) /* NEED RUN SERVER */
}
