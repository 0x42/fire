#include "../src/log/robolog.h"
#include "unity/src/unity.h"

void test_borobLogNotInit()
{
	int ans = bo_robLog("Hello world");
	TEST_ASSERT_EQUAL(-1, ans);
}

void test_borobLog()
{
	char *f = "robot_test.log";
	char *f_old = "robot_old_test.log";
	int ans = -1;
	bo_robLogInit(f, f_old, 0, 100);
	ans = bo_robLog("TEST MESSAGE");
	TEST_ASSERT_EQUAL(1, ans);
	remove(f);
	remove(f_old);
}

void test_borobLog1000()
{
	char *f = "robot_test.log";
	char *f_old = "robot_old_test.log";
	int ans = -1;
	bo_robLogInit(f, f_old, 0, 1000);
	int i = 0;
	for(;i < 1001; i++) {
		ans = bo_robLog("TEST MESSAGE");
		if(ans == -1) break;
	}
	TEST_ASSERT_EQUAL(1, ans);
	int nrow = readNRow(f);
	int nrow_old = readNRow(f_old);
	int flag = 0;
	if(nrow_old == 1000 && nrow == 1) flag = 1;
	TEST_ASSERT_TRUE(flag);
	remove(f);
	remove(f_old);
}

void test_borobLog10000()
{
	char *f = "robot_test.log";
	char *f_old = "robot_old_test.log";
	int ans = -1;
	bo_robLogInit(f, f_old, 0, 10);
	int i = 0;
	char strI[10];
	for(;i < 10001; i++) {
		sprintf(strI, "%d", i);
		ans = bo_robLog(strI);
		if(ans == -1) break;
	}
	TEST_ASSERT_EQUAL(1, ans);
	int nrow = readNRow(f);
	int nrow_old = readNRow(f_old);
	int flag = 0;
	if( (nrow_old == 10)&( nrow == 1) ) flag = 1;
	TEST_ASSERT_TRUE(flag);
	remove(f);
	remove(f_old);
}