#include "unity_fixture.h"

TEST_GROUP_RUNNER(master)
{
	// TAB ROUTE
//	RUN_TEST_CASE(master, boMasterCoreTest);
//	RUN_TEST_CASE(master, boMasterCoreTabTest);
//	RUN_TEST_CASE(master, boMasterCoreBadTabTest);
//	RUN_TEST_CASE(master, sendNULLTest);		/* NEED RUN SERVER */
//	RUN_TEST_CASE(master, chkSockTest);		/* NEED RUN SERVER */
//	RUN_TEST_CASE(master, chkAskTest);		/* NEED RUN SERVER */
//	RUN_TEST_CASE(master, boMasterSEND_TR);		/* NEED RUN SERVER */
	// LOG
	RUN_TEST_CASE(master, sendLogTest);		/* NEED RUN SERVER */
//	RUN_TEST_CASE(master, sendGetLogTest);		/* NEED RUN SERVER */
//	RUN_TEST_CASE(master, getNullLogTest);		/* NEED RUN SERVER */

//	Запускать при выкл др тестах
//	RUN_TEST_CASE(master, set2000getLast)		/* NEED RUN SERVER */

}
