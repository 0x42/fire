#include "logging.h"
// флаг проверки инициализации
static int initFlg = 0;
// имя лог файла
static char *logFileName = 0;

void loggingERROR(char *msg);
char *setERR(int *flag);
//void writeSysLog();
void writeSysLog(char *funcName, char *errTxt, char *msg);

/* иниц-ия получение имени файла для логирования*/
void loggingINIT()
{
	debugPrint("loggingINIT run\n");
	initFlg = 1;
	logFileName = "fire.log";
	debugPrint("initFlg = %d\nlogFileName = %s\n", initFlg, logFileName);
}

/* выводим в logFileName инфор-ое сообщение*/
void loggingINFO(char *msg)
{
	char *head = " INFO ";
	char *errTxt = "";
	int err = 0;
	if(initFlg) {
		FILE *file = fopen(logFileName, "r");
		if(file) {
			if(fprintf(file, "%s%s\n", head, msg) < 0)
				errTxt = setERR(&err);
			if(fclose(file) < 0) {
				errTxt = setERR(&err);
				msg = "when try close file!";
			}
		} else 	errTxt = setERR(&err);
	} else {
		errTxt = "run loggingINFO() before loggingINIT() when try write:\n";
		err = 1;
	}	
	if(err) writeSysLog("loggingINFO()", "123", errTxt, msg);
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
