#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "unity/src/unity.h"
#include "../src/log/logging.h"

extern int readNRow(const char *fname);
extern int writeNRow(FILE *f, char *s, int n);
extern int delOldFile(char *fname);
extern void sysErr();
extern void bo_setLogParam(char *fname, char *oldfname, int nrow, int maxrow);

void bo_log_writeNullMsg()
{
	char *msg = NULL;
	char *f = "test.log";
	char *f_old = "test_old.log";
	int ans = -1;
	bo_setLogParam(f, f_old, 0, 10);
	ans = bo_log(msg);
	remove(f);
	bo_resetLogInit();
	TEST_ASSERT_EQUAL(1, ans);
}

void bo_log_writeEmptyMsg()
{
	char *msg = "";
	char *f = "test.log";
	char *f_old = "test_old.log";
	int ans = -1;
	bo_setLogParam(f, f_old, 0, 10);
	ans = bo_log(msg);
	remove(f);
	bo_resetLogInit();
	TEST_ASSERT_EQUAL(1, ans);
}

void bo_log_writeOneChar()
{
	char *msg = "a";
	char *f = "test.log";
	char *f_old = "test_old.log";
	int ans = -1;
	bo_setLogParam(f, f_old, 0, 10);
	ans = bo_log(msg);
	remove(f);
	bo_resetLogInit();
	TEST_ASSERT_EQUAL(1, ans);
}

void delOldFile_test()
{
	char fname[] = "ringfile(1)_test.log";
        int err = 0;
	FILE *f = fopen(fname, "a+");
	fclose(f);
	if(delOldFile(fname) < 0) printf(" ERROR ");
	if( (f = fopen(fname, "r")) == NULL) err = 1;
	else err = 0;
	TEST_ASSERT_TRUE(err);
}

void testRead5Row()
{
        char *fname = "NRow.test"; 
        FILE *f = fopen(fname, "w");
        int nrow = -1;
        if(f != NULL) {
		fprintf(f, "\n");
                fprintf(f, " \n");
                fprintf(f, "a  a \n");
                fprintf(f, "\n");
                fprintf(f, " a \n");
                fclose(f);
                nrow = readNRow(fname);
        }
        remove(fname);
        TEST_ASSERT_EQUAL(5, nrow);
}

void testRead1Msg()
{
    char *logFileName = "msglog.test";
    int nrow = 0;
    bo_setLogParam(logFileName, "", 0, 1000);
    bo_log("Hello world");
    nrow = readNRow(logFileName);
    remove(logFileName);
    bo_resetLogInit();
    TEST_ASSERT_EQUAL(1, nrow);
}

void testReadNRowWithoutFile()
{
        char *fname = "EmptyROW.test";
        int nrow = -1;
        nrow = readNRow(fname);
        TEST_ASSERT_EQUAL(0, nrow);
        remove(fname);
}

void isBigLogSize_test1000row()
{
	char *log = "log.test";
	char *logOld = "logold.test";
	int flag = 0;
	int i;
	int oldRow, curRow;
	bo_setLogParam(log, logOld, 0, 1000);
	remove(logOld);
	remove(log);
	for(i = 0; i < 1001; i++) {
		bo_log(" Hello world ");
	}
	oldRow = readNRow(logOld);
	curRow = readNRow(log);
//	dbgout("oldRow = %d\n", oldRow);
//	dbgout("curRow = %d\n", curRow);
	if(oldRow == 1000 && curRow == 1) flag = 1;
	TEST_ASSERT_TRUE(flag);
	bo_resetLogInit();
	remove(logOld);
	remove(log);
}

void isBigLogSize_test100000row()
{
	char *log = "log.test";
	char *logOld = "logold.test";
	int flag = 0;
	int i, oldRow, curRow;
	bo_setLogParam(log, logOld, 0, 1000);
	remove(logOld);
	remove(log);
	for(i = 0; i < 100001; i++) {
		bo_log(" Hello world ");
	}
	oldRow = readNRow(logOld);
	curRow = readNRow(log);
//	dbgout("oldRow = %d\n", oldRow);
//	dbgout("curRow = %d\n", curRow);
	if(oldRow == 1000 && curRow == 1) flag = 1;
	TEST_ASSERT_TRUE(flag);
	bo_resetLogInit();
	remove(logOld);
	remove(log);
}

void testsysErr()
{	
	int p_int = 100;
	char *p_str = "Hello";
	float p_float = 3.14;
	sysErr("testsysErr() int=%d char=%c str=%s float=%f", 
		p_int, p_str, p_float);
	TEST_ASSERT_TRUE(1);
}

void testsysErrBADParam()
{
	sysErr("testsysErrBADParam() %s %s", 0, 0);
	TEST_ASSERT_TRUE(1);
}
/* [0x42] */