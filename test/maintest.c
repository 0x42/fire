/*
 * Тестирование производится с испол - ем библиотеки Unity
 * http://sourceforge.net/projects/unity/
 * домашняя страница проекта:
 * http://throwtheswitch.org/white-papers/unity-intro.html
 * запуск тестов производится командой 
 * $ make check 
 */

#include <stdio.h>
#include <stdlib.h>
#include "unity/src/unity.h"
#include "../src/log/logging.h"
#include "../src/log/robolog.h"
#include "../src/tools/linkedlist.h"

extern void bo_log_writeNullMsg();
extern void bo_log_writeOneChar();
extern void bo_log_writeEmptyMsg();

extern void delOldFile_test();
extern void isBigLogSize_test1000row();
extern void isBigLogSize_test100000row();
extern void testRead5Row();
extern void testReadNRowWithoutFile();
extern void testRead1Msg();
extern void testsysErr();
extern void testsysErrBADParam();
extern void test_isBigLogSize_BADParam();
void setUp(void) {}
void tearDown(void) {}

extern void test_borobLogNotInit();
extern void test_borobLog();
extern void test_borobLog1000();
extern void test_borobLog10000();

extern void initdel_test();
extern void crt_lliststress_test();

int main(int argc, char** argv)
{
	
	RUN_TEST(bo_log_writeNullMsg, 14);
	RUN_TEST(bo_log_writeEmptyMsg, 21);
	RUN_TEST(bo_log_writeOneChar, 28);
	RUN_TEST(delOldFile_test, 35);
	RUN_TEST(testRead5Row, 47);
	RUN_TEST(testReadNRowWithoutFile, 77);
	RUN_TEST(testRead1Msg, 65);
	RUN_TEST(isBigLogSize_test1000row, 130);
	RUN_TEST(isBigLogSize_test100000row, 149);
	RUN_TEST(testsysErr, 134);
	RUN_TEST(testsysErrBADParam, 143);
	RUN_TEST(test_borobLogNotInit, 3);
	RUN_TEST(test_borobLog, 10);
	RUN_TEST(test_borobLog1000, 22);
	RUN_TEST(test_borobLog10000, 43);
	RUN_TEST(initdel_test, 8);
	RUN_TEST(crt_lliststress_test, 22);
	return 0;
}

/* [0x42] */