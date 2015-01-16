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
	struct bo_sock *val = NULL;
	int s[] = {1,2,3,4,5,6,7,8,9,10};
	int exec = -1;
	int pos = -1;
	int i = 0;
	int ans = 1;
	printf("simpleTest ... \n");
	/* 
	 * добавляем один элемент один снимаем
	 */
	for(; i < 10; i++) {
		exec = bo_addll(list, s[i]);
		if(exec == -1) {
			ans = -1; 
			printf("list is full\n");
			break;
		}
		pos = bo_get_head(list);
		if(pos == -1) {
			printf("list is empty\n");
			ans = -1;
			break;
		} 

		exec = bo_get_val(list, &val, pos);
		if(val == NULL) {
			printf("val is NULL\n"); ans = -1; break;
		}
		
		if(exec != -1) {
			printf("error in list \n"); ans = -1; break;
		}
		
		if(val->sock != s[i]) { printf("list return bad param\n");
			printf(" error s[%d] != val[%d]\n", s[i], val->sock); 
			ans = -1; break;
		}
		
		bo_del_val(list, pos);
	}
	bo_del_lsock(list);
	
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(llist, overflowListTest)
{
	struct bo_llsock *list = bo_crtLLSock(10);
	struct bo_sock *val = NULL;
	int s[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14};
	int exec = -1;
	int i = 0;
	int ans = 1;
	printf("overflowListTest() ... \n");
	/* 
	 * добавляем 
	 */

	for(i = 0; i < 14; i++) {
		exec = bo_addll(list, s[i]);

		if(i < 9) {
			if(exec == -1) {
				ans = -1; 
				printf("list is full\n"); break;
			}
		} else {
			if(exec != -1) {
				bo_print_list(list);
				ans = -1;
				printf("list is not full\n"); break;
			}
		}
		
	}
	bo_del_lsock(list);
	
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(llist, checkDelValTest)
{
	struct bo_llsock *list = bo_crtLLSock(10);
	struct bo_sock *val = NULL;
	int s[] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14};
	int exec = -1;
	int i = 0;
	int ans = 1;
	int stop = 1;
	printf("checkDelValTest() ... \n");
	
	exec = bo_addll(list, 1);
	exec = bo_addll(list, 2);
	exec = bo_addll(list, 3);

	bo_del_val(list, 2);
	
	i = bo_get_head(list);
	
	exec = bo_get_val(list, &val, i);
	if(exec == -1) { ans = -1; printf("list is empty\n"); goto end; }
	if(val->sock != 1) { ans = -1; printf("bad param\n");goto end; }
	
	exec = bo_get_val(list, &val, exec);
	if(exec != -1) { ans = -1; printf("list is not empty\n"); goto end; }
	if(val->sock != 3) { ans = -1; printf("bad param\n");goto end; }
	
	end:
	bo_del_lsock(list);
	
	TEST_ASSERT_EQUAL(1, ans);
}

/* ************************************************************************** */
/* test bo_send_lst							      */
/* ************************************************************************** */
TEST(llist, sock_lst)
{

}