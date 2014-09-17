#include <stdio.h>
#include <errno.h>
#include "../../src/log/robolog.h"

int getNRow(const char *fname) 
{
	int nrow = 0;
	FILE *file = fopen(fname, "r");
	if(file) {
                while(!feof(file)) {
			char ch;
			if((ch = fgetc(file)) == '\n')
				nrow++;
                }
	}
	return nrow;
}

int robLog1000() 
{
	char c[10] = {0};
	int ans = -1, exec = -1, i = 0;
	int nrow = 0, nrowold = 0;
	char *f = "/var/log/robot.stress";
	char *f_old = "/var/log/robot_old.stress";
	bo_robLogInit(f, f_old, 0, 100);
	for(; i < 1000; i++) {
		sprintf(c, "%d", i);
		exec = bo_robLog(c);
		if(exec < 0) return -1;
	}
	nrow = getNRow(f);
	nrowold = getNRow(f_old);
	if(nrow == 1 && nrowold == 100) ans = 1;
	remove(f);
	remove(f_old);
	return ans;
}

/* 100x1024 = 100K
 * /var/log всего 115K нехватает места
 *  
 */
int crtFile1024Row()
{
	int ans = -1;
	int i = 0, j = 0;
	char *f = "/var/log/big.file";
	char c;
	FILE *file = fopen(f, "a+");
	for(; i < 1024; i++) {
		if(fprintf(file, "%d ->", i) < 0) {
			printf("crtFile1024Row() ERROR can't write data");
			goto exit;
		}
		for(j = 0; j < 100; j++) {
			if(fprintf(file, "01") < 0) {
				printf("crtFile1024Row() ERROR can't write data\n errno[%s]",
					strerrno(errno));
				goto exit;
			}
			if(fflush(file) != 0) {
				printf("crtFile1024Row() ERROR can't write data\nerrno[%s]\n"
					,strerrno(errno));
				goto exit;
			}
		}
		if(i==250 || i==500 || i==750) {
			printf("i = %d \n continue y/n: ", i);
			scanf("%c", &c);
			if(c == 'n') break;
		}
		if(fprintf(file, "\n") < 0) {
			printf("crtFile1024Row() ERROR can't write data");
			goto exit;
		}
	}
	ans = 1;
	exit:
	if(fclose(file)) {
		printf("int crtFile1024Row() errno[%s]", strerror(errno));
		ans = 1;
	};
	return ans;
}

int main() 
{
	printf("STRESS RUN\n");
//	if(robLog1000() < 0) printf("robLog1000() stress FAILED");
	if(crtFile1024Row() < 0) printf("crtFile1024Row() stress FAILED");
	printf("STRESS END\n");
	return 1;
}