#include "unity/src/unity.h"
#include "../src/tools/bmempool.h"

extern int b_broken();
extern int b_pool();

void b_broken_simpletest()
{
	b_broken();	
}

void b_pool_test()
{
	int ans = 1;
	struct obstack *stack = b_crtStack();
	if(stack != NULL) {
		char *str = (char *)b_allocInStack(stack, 10);
		if(str == NULL) {
			ans = -1;
		}
		b_delStack(stack);
	} else {
		ans = -1;
	}
	TEST_ASSERT_EQUAL(1, ans);
}

void b_pool_add100str()
{
	int ans = 1, i = 0;
	char *str = NULL, *str1 = NULL, *str2 = NULL;
	struct obstack *stack = b_crtStack();
	if(stack != NULL) {
		str1 = (char *)b_allocInStack(stack, 40);
		strcpy(str1, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");
		str2 = (char *)b_allocInStack(stack, 40);
		strcpy(str2, "bbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbbb");
		printf("str1 = %d\nstr2=%d\n", str1, str2);
		printf("str1=[%s]\nstr2=[%s]\n", str1, str2);
		for(; i < 200; i++) {
			str = (char *)b_allocInStack(stack, 40);
			if(str == NULL) {
				ans = -1;
				break;
			}
		}
		
		printf("str1 = %d\nstr2=%d\n", str1, str2);
		b_delStack(stack);
	} else {
		ans = -1;
	}
	TEST_ASSERT_EQUAL(1, ans);
}
