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
	char *buf;
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

void testReadNRow()
{
	wrtLog(" INFO ", "01234567890123456789012345678901234567890123456789\
			  01234567890123456789012345678901234567890123456789");
	int nrow = readNRow("ringfile.log");
}