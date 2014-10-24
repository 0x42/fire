#include "unity_fixture.h"

TEST_GROUP_RUNNER(bolog)
{
	RUN_TEST_CASE(bolog, bo_log_writeNullMsg);
	RUN_TEST_CASE(bolog, bo_log_writeEmptyMsg);
	RUN_TEST_CASE(bolog, bo_log_writeOneChar);
	RUN_TEST_CASE(bolog, delOldFile_test);
	RUN_TEST_CASE(bolog, testRead5Row);
	RUN_TEST_CASE(bolog, testRead1Msg);
	RUN_TEST_CASE(bolog, testReadNRowWithoutFile);
	RUN_TEST_CASE(bolog, isBigLogSize_test1000row);
	RUN_TEST_CASE(bolog, isBigLogSize_test100000row);

}
