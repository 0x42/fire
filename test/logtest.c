#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include "unity/src/unity.h"
#include "../src/log/logging.h"

extern int readNRow(const char *fname);
extern int writeNRow(FILE *f, char *s, int n);
extern void setLogParam(char *fname, char *oldfname, int nrow, int maxrow);
extern int delOldFile(char *fname);
extern void resetLogInit();

void loggingINFO_writeNullMsg()
{
	char *msg = NULL;
	int ans = loggingINFO(msg);
	TEST_ASSERT_EQUAL(1, ans);
}

void loggingINFO_writeOneChar()
{
	char *msg = "a";
	int ans = loggingINFO(msg);
	TEST_ASSERT_EQUAL(1, ans);
}

void loggingERROR_writeNullMsg()
{
	char *msg = NULL;
	int ans = loggingERROR(msg);
	TEST_ASSERT_EQUAL(1, ans);
}

void loggingERROR_writeOneChar()
{
	char *msg = "a";
	int ans = loggingERROR(msg);
	TEST_ASSERT_EQUAL(1, ans);
}

void delOldFile_test()
{
	char fname[] = "ringfile(1)_test.log";
        FILE *f = fopen(fname, "a+");
	fclose(f);
	if(delOldFile(fname) < 0) printf(" ERROR ");
	int err = 0;
	if( (f = fopen(fname, "r")) == NULL) err = 1;
	else err = 0;
	TEST_ASSERT_TRUE(err);
}

void loggingRingFile()
{
	FILE *file = fopen("ringfile.log","a+");
	fpos_t pos;
	int poss;
	char buf[10];
	int i = 1;
	printf("ringlog ...\n");
	while(!feof(file)) {
		if(fgets(buf, 10, file)) {
			printf("%d: [%s]\n", i, buf);
			if(i == 3) {
				fgetpos(file, &pos);
				poss = ftell(file);
			}
			i++;
		}
	}
	fclose(file);
	
	file = fopen("ringfile.log", "a+");
	fseek(file, poss, SEEK_SET);
//	rewind(file);
	for(i = 0; i < 3; i++) 
		fprintf(file,"b\n");
	
	fclose(file);
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
    setLogParam(logFileName, "", 0, 1000);
    loggingINFO("Hello world");
    int nrow = readNRow(logFileName);
    remove(logFileName);
    resetLogInit();
    TEST_ASSERT_EQUAL(1, nrow);
}

void testReadNRowFromEmptyFile()
{
        char *fname = "EmptyROW.test";
        FILE *f = fopen(fname, "w");
        fclose(f);
        int nrow = -1;
        nrow = readNRow(fname);
        TEST_ASSERT_EQUAL(0, nrow);
        remove(fname);
}

/**/
void isBigLogSize_test1000row()
{
	char *log = "log.test";
	char *logOld = "logold.test";
	setLogParam(log, logOld, 0, 1000);
	remove(logOld);
	remove(log);
	int i;
	for(i = 0; i < 1001; i++) {
		loggingINFO(" Hello world ");
	}
	int flag = 0;
	int oldRow = readNRow(logOld);
	int curRow = readNRow(log);
	dbgout("oldRow = %d\n", oldRow);
	dbgout("curRow = %d\n", curRow);
	if(oldRow == 1000 && curRow == 1) flag = 1;
	TEST_ASSERT_TRUE(flag);
	resetLogInit();
	remove(logOld);
	remove(log);
}








