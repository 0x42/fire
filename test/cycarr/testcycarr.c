#include "unity_fixture.h"
#include "../../src/nettcp/bo_cycle_arr.h"

TEST_GROUP(cycarr);

TEST_SETUP(cycarr) {}

TEST_TEAR_DOWN(cycarr) {}

TEST(cycarr, addGet) 
{
	int ans = -1;
	unsigned char val[] = "0x42:Hello World";
	unsigned char buf[BO_ARR_ITEM_VAL];
	int i = 0;
	int exec = -1;
	printf("addGet ... run\n");
	struct bo_cycle_arr *arr = bo_initCycleArr(10);
	if(arr == NULL) { printf(" init error\n"); goto exit;}
	
	exec = bo_cycle_arr_add(arr, val, strlen(val)); 
	if(exec == -1) { printf(" add error\n "); goto exit;}
	
	exec = bo_cycle_arr_get(arr, buf, 0);
	if(exec < 1) { printf("get error\n"); goto exit;}
	
	for(; i < exec; i++) {
		if(buf[i] != val[i]) { 
			printf("compare error \n"); 
			goto exit;
		}
	}
	ans = 1;
	exit:
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(cycarr, addGet2) 
{
	int ans = -1;
	unsigned char val[] = "0x42:Hello World";
	unsigned char buf[BO_ARR_ITEM_VAL];
	int i = 0;
	int exec = -1;
	printf("addGet2 ... run\n");
	struct bo_cycle_arr *arr = bo_initCycleArr(10);
	if(arr == NULL) { printf(" init error\n"); goto exit;}
	
	exec = bo_cycle_arr_add(arr, val, strlen(val)); 
	if(exec == -1) { printf(" add error\n "); goto exit;}
	
	exec = bo_cycle_arr_get(arr, buf, 1);
	if(exec != 0) { printf("get error\n"); goto exit;}
	
	ans = 1;
	exit:
	TEST_ASSERT_EQUAL(1, ans);
}