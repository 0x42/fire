#include "../src/tools/linkedlist.h"
#include "unity/src/unity.h"

extern LinkedList *initLList(char *errTxt);
extern int addLLItem(LinkedList *llist, void *obj, int size, char *errTxt);
extern void delLList(LinkedList *llist);

void initdel_test()
{
	int flag = 1;
	char *errTxt = NULL;
	LinkedList *ll = initLList(errTxt);
	if(ll == NULL) {
		flag = -1;
	} else {
		delLList(ll);
	}
	if(ll != NULL) flag = -1;
	TEST_ASSERT_TRUE(flag);
}

void crt_lliststress_test()
{
	int flag = -1;
	char *errTxt = NULL;
	LinkedList *ll = initLList(errTxt);
	LLItem *llitem = NULL;
	if(ll == NULL) {
		flag = -1;
	} else {
		int i = 0;
		int *ptr_i;
		for(; i < 10; i++) {
			ptr_i  = malloc(sizeof(int));
			*ptr_i = i;
			addLLItem(ll, (void*)ptr_i, sizeof(i), errTxt);
		}
		if(ll->length != 10) flag = -1;
		// сравнение списка
		llitem = ll->Head;
		int val = 0;
		int *ptr = 0;
		for(i = 0; i < 10; i++) {
			ptr = (int *)llitem->Object;
			val = *ptr;
			printf("%d = %d \n", i, val);
			llitem = llitem->Next;
		}
		delLList(ll);
	}
	if(ll != NULL) flag = -1;
	TEST_ASSERT_TRUE(flag);
}
