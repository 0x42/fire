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

TEST(cycarr, addTen)
{
	int ans = -1; int exec = -1;
	printf("addTen ... run\n");
	int i = 0; char val[10][20] = {0}; char buf[20] = {0}; 
	char *str;
	for(i = 0; i < 10; i++) {
		str = val[i];
		sprintf(str, "%d %s", i, "0x42");
		printf("val[%s]\n", val[i]);
	}
	
	struct bo_cycle_arr *arr = bo_initCycleArr(10);
	
	for(i = 0; i < 10; i++) {
		exec = bo_cycle_arr_add(arr, val[i], strlen(val[i]));
		if(exec == -1) { printf(" add error\n "); goto exit;}
	}
	
	exec = bo_cycle_arr_get(arr, buf, 8);
	if(exec < 1) { printf("get error\n"); goto exit;}
	for(i = 0; i < exec; i++) {
		if(buf[i] != val[8][i]) { 
			printf("compare 8 error \n"); 
			goto exit;
		}
	}
	
	exec = bo_cycle_arr_get(arr, buf, 9);
	if(exec < 1) { printf("get error\n"); goto exit;}
	for(i = 0; i < exec; i++) {
		if(buf[i] != val[9][i]) { 
			printf("compare 9 error \n"); 
			goto exit;
		}
	}
	ans = 1;
	exit:
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(cycarr, add100getLast)
{
	printf("add100getLast ... run\n");
	int ans = -1; int exec = -1;
	int i = 0; char val[100][20] = {0}; char buf[20] = {0}; 
	char *str;
	
	for(i = 0; i < 100; i++) {
		str = val[i];
		sprintf(str, "%d %s", i, "0x42");
	}
	
	struct bo_cycle_arr *arr = bo_initCycleArr(10);
	
	for(i = 0; i < 100; i++) {
		exec = bo_cycle_arr_add(arr, val[i], strlen(val[i]));
		if(exec == -1) { printf(" add error\n "); goto exit;}
	}
	int j = 0;
	exec = bo_cycle_arr_get(arr, buf, 8);
	if(exec < 1) { printf("get error\n"); goto exit;}
	for(i = 0; i < exec; i++) {
		if(buf[i] != val[98][i]) {
			printf("buf[");
			for(j = 0; j < exec; j++) {
				printf("%c", buf[j]);
			}
			printf("]\n val[");
			for(j = 0; j < exec; j++) {
				printf("%c", val[98][j]);
			}
			printf("]\ncompare 8 error \n"); 
			goto exit;
		}
	}
	
	exec = bo_cycle_arr_get(arr, buf, 9);
	if(exec < 1) { printf("get error\n"); goto exit;}
	for(i = 0; i < exec; i++) {
		if(buf[i] != val[99][i]) { 
			printf("compare 9 error \n"); 
			goto exit;
		}
	}
	ans = 1;
	exit:
	TEST_ASSERT_EQUAL(1, ans);
}