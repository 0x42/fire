#include "logging.h"

// имя лог файла
static char *logFileName = 0;

void loggingERROR(char *msg);
char *setERR(int *flag);
void getTimeNow(char * timeBuf, int sizeBuf);
void writeSysLog(char *funcName, char *errTxt, char *msg);

/* иниц-ия получение имени файла для логирования*/
void loggingINIT()
{
	debugPrint("loggingINIT run\n");
	logFileName = "fire.log";
	debugPrint("logFileName = %s\n", logFileName);
}

/* выводим в logFileName инфор-ое сообщение
   @return = возвр -1 в случае ошибки, >0 вслучае успешной записи
*/
int loggingINFO(char *msg)
{
	if(msg == NULL) msg = "";
	// флаг проверки инициализации
	static int initFlg = -1;
	char *head = " INFO ";
	char *errTxt = "";
	int err = 0;
	int ans = 0;
	char timeBuf[80] = {0};
	if(initFlg < 0) {
		loggingINIT();
		initFlg = 1;
	}
	if(initFlg) {
		FILE *file = fopen(logFileName, "a+");
		getTimeNow(timeBuf, sizeof(timeBuf));
		if(file) {
			if(fprintf(file, "%s%s%s\n", timeBuf, head, msg) < 0)
				errTxt = setERR(&err);
			if(fclose(file) < 0) {
				errTxt = setERR(&err);
				msg = "when try close file!";
			}
		} else 	errTxt = setERR(&err);
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
void loggingERROR(char *msg)
{
	char *head = " ERROR ";
	printf("%s->%s", head, msg);
}

/* сохраняем инфор о ошибках в системный лог /var/log/syslog*/
void writeSysLog(char *funcName, char *errTxt, char *msg) 
{
	debugPrint("-----\nwriteSysLog() in syslog wrote message\n-----");
	/* откр соед к Linux'му логгеру
	 * указывает PID приложения в каждом сообщение
	   тип приложения пользовательское*/
	openlog("FIREROBOTS-NETWORK", LOG_PID, LOG_USER);
	syslog(LOG_MAKEPRI(LOG_USER, LOG_ERR), 
		"%s logFileName[%s] errno[%s] msg[%s]",
		funcName, logFileName, errTxt, msg);
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
	if(sizeBuf < 20) {
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