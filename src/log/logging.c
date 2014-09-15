#include "logging.h"


char *setERR(int *flag);
void getTimeNow(char * timeBuf, int sizeBuf);
void writeSysLog(char *funcName, char *errTxt, char *msg);
int wrtLog(char *head, char *msg, char *errTxt);
int readNRow(const char *fname);
void isBigLogSize();
int delOldFile(char *fname);
void sysError(const char *funcMsg, const char *msg);
/* ========================================================================== */
static struct {
    /* имя лог файла*/
    char *name;
    char *oldname;
    /*текущ. кол-во строк в лог файле*/
    int nrow;
    /*макс кол-во строк в лог файле*/
    int maxrow;
} log;

static int logInit = -1;

void setLogParam(char *fname, char *oldfname, int nrow, int maxrow)
{
        log.name = fname;
        log.oldname = oldfname;
	log.nrow = nrow;
	log.maxrow = maxrow;
	logInit = 1;
}

void resetLogInit()
{
	logInit = -1;
}

/* ========================================================================== */
/* иниц-ия получение имени файла для логирования*/
void loggingINIT()
{
	dbgout("loggingINIT run\n");
	/* ЧИТАТЬ С КОНФ ФАЙЛА*/
	log.name = "ringfile.log";
	log.oldname = "ringfile_OLD.log";
	log.maxrow = 1000;
	/* ================= */
        logInit = 1;
	log.nrow = readNRow(log.name);
	dbgout("logFileName = %s\n", log.name);
}

/* выводим в logFileName инфор-ое сообщение
   @return = возвр -1 в случае ошибки, >0 вслучае успешной записи
*/
int logInfo(char *msg)
{
	char *head = " INFO ";
	char *errTxt = "";
	int err = 0;
	int ans = 0;
	
	if(msg == NULL) msg = "";
	if(logInit < 0) {
		loggingINIT();
	}
	if(logInit) {
		if (wrtLog(head, msg, errTxt) < 0) err = 1;
	} else {
		errTxt = "loggingINFO() can't run loggingINIT() when try write:\n";
		err = 1;
	}	
	if(err) {
		writeSysLog("loggingINFO()", errTxt, msg);
		ans = -1;
	} else ans = 1;
	return ans;
}

/* выводим error сообщение в лог */
int logErr(char *msg)
{
	char *head = " ERROR ";
	char *errTxt = "";
	int err = 0;
	int ans = 0;

	if(msg == NULL) msg = "";
	if(logInit < 0) {
		loggingINIT();
	}
	if(logInit) {
		if (wrtLog(head, msg, errTxt) < 0) err = 1;
	} else {
		errTxt = "loggingERROR() can't run loggingINIT() when try write:\n";
		err = 1;
	}	
	if(err) {
		writeSysLog("loggingERROR()", errTxt, msg);
		ans = -1;
	} else ans = 1;
	return ans;
}

int wrtLog(char *head, char *msg, char *errTxt)
{
	int ans = 1;
	char timeBuf[30] = {0};
	FILE *file;
	
	isBigLogSize();
	file = fopen(log.name, "a+");
	getTimeNow(timeBuf, sizeof(timeBuf));
	if(file) {
		if(fprintf(file, "%s%s%s\n", timeBuf, head, msg) < 0)
			errTxt = setERR(&ans);
		else log.nrow++;
		if(fclose(file) < 0) {
			errTxt = setERR(&ans);
			msg = "when try close file!";
		}
	} else 	errTxt = setERR(&ans);
	return ans;
}

/*@return - возвр. кол-во строк в лог файле*/
int readNRow(const char *fname) 
{
	int nrow = 0;
	FILE *file = fopen(fname, "r");
	if(file) {
                while(!feof(file)) {
			char ch;
			if((ch = fgetc(file)) == '\n')
				nrow++;
                }
	} else {
		sysError("readNRow()", fname);
                nrow = -1;
	}
	return nrow;
}

/* сохраняем инфор о ошибках в системный лог /var/log/syslog*/
void writeSysLog(char *funcName, char *errTxt, char *msg) 
{
	dbgout("-----\nwriteSysLog() in syslog wrote message\n-----");
	/* откр соед к Linux'му логгеру
	 * указывает PID приложения в каждом сообщение
	   тип приложения пользовательское*/
	openlog("FIREROBOTS-NETWORK", LOG_PID, LOG_USER);
	syslog(LOG_MAKEPRI(LOG_USER, LOG_ERR), 
		"%s logFileName[%s] errno[%s] msg[%s]",
		funcName, log.name, errTxt, msg);
	closelog();
}

void sysError(const char *funcName, const char *errTxt) 
{
	dbgout("\n!!! CRITICAL ERROR !!!\n");
	if (errTxt == NULL) errTxt = "";
	dbgout("in ->%s\nerr[%s]\n", funcName, errTxt);
}

/* проверяем размер файла log.name если nrow = maxrow то log.name
 *  сохраняем c именем log.oldname. Создаем пустой файл c именем log.name  */
void isBigLogSize()
{
	if(log.nrow >= log.maxrow) {
		if(delOldFile(log.oldname) > 0 ) { 
			if(rename(log.name, log.oldname) != 0) {
				sysError("isBigLogSize()", 
					strerror(errno));
			} else log.nrow = 0;
		}
	}
}

int delOldFile(char *fname) 
{
	int err = -1;
	char *errTxt = NULL;
	int ans = 1;
	FILE *file = fopen(fname, "r");
	if(file != NULL) {
		if(fclose(file) < 0) {
			errTxt = setERR(&err);
		} else {
			if(remove(fname) < 0) {
				errTxt = setERR(&err);
			} 
		}
	}
	if(err > 0) {
		sysError("delOldFile()", errTxt);
		ans = -1;
	}
	return ans;
}

/* сохраняем текст errno - чтобы не затЁрла др функция,
   устанавливаем флаг что ошибка произошла*/
char * setERR(int *flag) 
{
	char *msg;
	*flag = 1;
	msg = strerror(errno);
	return msg;
}

/* возвращает дату и время в заданном формате день-месяц-год час:мин:сек
   size - размер буфер  */
void getTimeNow(char *timeStr, int sizeBuf) 
{
	struct tm *ptr;
	time_t lt;

	if(sizeBuf < 30) {
		/*не пишем лог тк ошибка возможна только на этапе кодирования*/
		printf("getTimeNow() - ERROR - массив не достаточного размера\n");
		return;
	}
	/*возвр текущ время системы*/
	lt = time(NULL);
	/*преобразует в структуру tm */
	ptr = localtime(&lt);
	if(strftime(timeStr, sizeBuf, "%d-%m-%Y %H:%M:%S", ptr) == 0)
		/*не пишем лог тк ошибка возможна только на этапе кодирования*/
		printf("getTimeNow() - ERROR - массив не достаточного размера\n"); 
}