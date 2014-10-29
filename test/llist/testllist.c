#include <stdlib.h>
#include <string.h>
#include "../../src/tools/listsock.h"
#include "unity_fixture.h"

TEST_GROUP(llist);

TEST_SETUP(llist) {}

TEST_TEAR_DOWN(llist) {}

TEST(llist, simpleTest)
{
 	struct bo_llsock *list = bo_crtLLSock(10);
	printf("crt llist ... [%d]\n", sizeof(struct bo_llsock));
	int s[] = {1,2,3,4,5,6,7,8};
	int exec = -1;
	int i = 0;
	
	for(; i < sizeof(s); i++) {
		exec = bo_addll(s);
	}
	
	bo_del_lsock(list);
}