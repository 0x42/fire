
#include "unity_fixture.h"

TEST_GROUP_RUNNER(ocfg)
{
	RUN_TEST_CASE(ocfg, CfgLoad);
	RUN_TEST_CASE(ocfg, CfgParseLine);
	RUN_TEST_CASE(ocfg, CfgGetString);
	RUN_TEST_CASE(ocfg, CfgGetStringDefvalue);
	RUN_TEST_CASE(ocfg, CfgGetInt);
	RUN_TEST_CASE(ocfg, CfgGetIntDefvalue);
	RUN_TEST_CASE(ocfg, CfgCreateFile);
}

