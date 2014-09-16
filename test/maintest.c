/*
 * Тестирование производится с испол - ем библиотеки Unity
 * http://sourceforge.net/projects/unity/
 * домашняя страница проекта:
 * http://throwtheswitch.org/white-papers/unity-intro.html
 * запуск тестов производится командой 
 * $ make runtest 
 */

#include <stdio.h>
#include <stdlib.h>
#include "unity/src/unity.h"
#include "../src/log/logging.h"

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
	return 0;
}

/* [0x42] */