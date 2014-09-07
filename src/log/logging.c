#include "logging.h"

static struct {
    /* имя лог файла*/
    char *name;
    /*текущ. кол-во строк в лог файле*/
    int nrow;
} log;

static int logInit = -1;

char *setERR(int *flag);
void getTimeNow(char * timeBuf, int sizeBuf);
void writeSysLog(char *funcName, char *errTxt, char *msg);
int wrtLog(char *head, char *msg, char *errTxt);

void setLogFileName(char *fname, int size)
{
        log.name = fname;
        logInit = 1;
}

/* иниц-ия получение имени файла для логирования*/
void loggingINIT()
{
	dbgout("loggingINIT run\n");
	log.name = "ringfile.log";
        logInit = 1;
//	nrow = readNRow(logFileName);
	dbgout("logFileName = %s\n", log.name);
}

/* выводим в logFileName инфор-ое сообщение
   @return = возвр -1 в случае ошибки, >0 вслучае успешной записи
*/
int loggingINFO(char *msg)
{
	if(msg == NULL) msg = "";
	char *head = " INFO ";
	char *errTxt = "";
	int err = 0;
	int ans = 0;
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
int loggingERROR(char *msg)
{
	char *head = " ERROR ";
	if(msg == NULL) msg = "";
	char *errTxt = "";
	int err = 0;
	int ans = 0;
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
	FILE *file = fopen(log.name, "a+");
	getTimeNow(timeBuf, sizeof(timeBuf));
	if(file) {
		if(fprintf(file, "%s%s%s\n", timeBuf, head, msg) < 0)
			errTxt = setERR(&ans);
		if(fclose(file) < 0) {
			errTxt = setERR(&ans);
			msg = "when try close file!";
		}
	} else 	errTxt = setERR(&ans);
	return ans;
}

/* читает кол-во строк в лог файле*/
int readNRow(const char *fname) 
{
	int nrow = 0;
	FILE *file = fopen(fname, "r");
	if(file) {
                char str[10];
                while(!feof(file)) {
                        if( fscanf(file, "%10s", str) == 1)
                                nrow++;
                }
	} else {
		dbgout("readNRow() fname = %s error", fname);
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

/* сохраняем текст errno - чтобы не затЁрла др функция,
   устанавливаем флаг что ошибка произошла*/
char * setERR(int *flag) 
{
	*flag = 1;
	char *msg = strerror(errno);
	return msg;
}

/* возвращает дату и время в заданном формате день-месяц-год час:мин:сек
   size - размер буфер  */
void getTimeNow(char *timeStr, int sizeBuf) 
{
	if(sizeBuf < 30) {
		/*не пишем лог тк ошибка возможна только на этапе кодирования*/
		printf("getTimeNow() - ERROR - массив не достаточного размера\n");
		return;
	}
	struct tm *ptr;
	time_t lt;
	/*возвр текущ время системы*/
	lt = time(NULL);
	/*преобразует в структуру tm */
	ptr = localtime(&lt);
	if(strftime(timeStr, sizeBuf, "%d-%m-%Y %H:%M:%S", ptr) == 0)
		/*не пишем лог тк ошибка возможна только на этапе кодирования*/
		printf("getTimeNow() - ERROR - массив не достаточного размера\n"); 
}