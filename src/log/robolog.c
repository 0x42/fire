#include "robolog.h"

static struct {
    char *name;
    char *oldname;
    int nrow;
    int maxrow;    
} robolog = {0};
/* ----------------------------------------------------------------------------
 * @brief		устан параметры вместо default-ых
 * @fname		имя лог файла в который будет сохр лог в случае 
 *			отказа Ethernet
 * @foldname		имя лог файла в который будет записана послед maxrow
 * @row			текущ кол-во строк в лог файле
 * @maxrow		макс число строк в логе
 */
void bo_robLogInit(char *fname, char *foldname, int row, int maxrow)
{
	robolog.name = fname;
	robolog.oldname = foldname;
	robolog.nrow = row;
	robolog.maxrow = maxrow;
}
/* ----------------------------------------------------------------------------
 * @brief		запись в лог файл если сеть не работает
 * @msg			лог
 * @return		-1 error 1 ok
 */
int bo_robLog(char *msg)
{
	int ans = 1;
	char *errTxt = NULL;
	FILE *file = NULL;
	char timeBuf[40] = {0};
	if(bo_isBigLogSize(&robolog.nrow, robolog.maxrow, robolog.name, 
		robolog.oldname) > 0) {
		file = fopen(robolog.name, "a+");
		bo_getTimeNow(timeBuf, sizeof(timeBuf));
		if(file) {
			if(fprintf(file, "%s %s\n", timeBuf, msg) < 0) {
				errTxt = strerror(errno);
				ans = -1;
			} else robolog.nrow++;

			if(fclose(file) < 0) {
				errTxt = strerror(errno);
				ans = -1;
			}
		} else {
			errTxt = strerror(errno);
			ans = -1;
		}
	} else {
		errTxt = "bo_isBigLogSize() failed";
		ans = -1;
	}
	if(ans == -1) {
		bo_log("bo_robLog() errno[%s] msg[%s]", errTxt, msg);
	}
	return ans;
}