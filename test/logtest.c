#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>

#include "unity/src/unity.h"
#include "../src/log/logging.h"

/*
 * 
 */

//extern int loggingINFO(char *msg);
extern int readNRow(const char *fname);
extern int writeNRow(FILE *f, char *s, int n);

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

void delOldLog()
{
	char fname[] = "ringfile(1).log";
	struct stat *buf;
        // obtain info about file
	if(stat(fname, buf) == 0) {
		printf("file exist %s\n", fname);
		if(remove(fname) < 0) {
			printf("can't del file: %s\n", fname);
		}
	} else {
		printf("file no exist %s\n", fname);
	}
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
                fprintf(f, " \n");
                fprintf(f, "a  a \n");
                fprintf(f, "\n");
                fprintf(f, " a \n");
                fclose(f);
                nrow = readNRow(fname);
        }
        remove(fname);
        TEST_ASSERT_EQUAL(3, nrow);
        
}

void testRead1Msg()
{
    char *logFileName = "msglog.test";
    setLogFileName(logFileName, 11);
    loggingINFO("Hello world");
    int nrow = readNRow(logFileName);
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










