#include "unity_fixture.h"

static void RunAllTests(void)
{
	RUN_TEST_GROUP(master);
}

int main(int argc, char * argv[])
{
	return UnityMain(argc, argv, RunAllTests);
}
