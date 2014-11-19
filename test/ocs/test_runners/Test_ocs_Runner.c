
#include "unity_fixture.h"

TEST_GROUP_RUNNER(ocs)
{
	RUN_TEST_CASE(ocs, CsReaderSt1_FFC0);
	RUN_TEST_CASE(ocs, CsReaderSt1_DC);
	RUN_TEST_CASE(ocs, CsReaderSt1_DD);
	RUN_TEST_CASE(ocs, CsReaderSt2_DB);
	RUN_TEST_CASE(ocs, CsReaderSt4_D0);
	RUN_TEST_CASE(ocs, CsReaderSt3_000);
	RUN_TEST_CASE(ocs, CsReaderSt4_000);
	RUN_TEST_CASE(ocs, CsWriter_123456789);
	RUN_TEST_CASE(ocs, CsWriter_738);
	RUN_TEST_CASE(ocs, CsWriter_C0DBC0);
	RUN_TEST_CASE(ocs, CsWriter_FFFFF);
	RUN_TEST_CASE(ocs, CsWriter_000);
}

