
#include "unity_fixture.h"

TEST_GROUP_RUNNER(ort)
{
	RUN_TEST_CASE(ort, RtLoad);
	RUN_TEST_CASE(ort, RtGetPort);
	RUN_TEST_CASE(ort, RtGetIP);
	RUN_TEST_CASE(ort, RtCreateFile1);
	RUN_TEST_CASE(ort, RtCreateFile2);
}

