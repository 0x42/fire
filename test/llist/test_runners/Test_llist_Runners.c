#include "unity_fixture.h"

TEST_GROUP_RUNNER(llist)
{
	RUN_TEST_CASE(llist, simpleTest);
	RUN_TEST_CASE(llist, overflowListTest);
	RUN_TEST_CASE(llist, checkDelValTest);
	
	RUN_TEST_CASE(llist, test_sock_lst_add);
	RUN_TEST_CASE(llist, test_sock_lst_add2);
}
