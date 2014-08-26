#include "logging.h"
// флаг проверки инициализации
static int initFlg = 0;
// имя лог файла
static char *logFileName = 0;

// иниц-ия получение имени файла для логирования
void loggingINIT()
{
	debugPrint("loggingINIT run\n");
	initFlg = 1;
	logFileName = "fire.log";
	debugPrint("initFlg = %d\nlogFileName = %s\n", initFlg, logFileName);
}

// выводи в logFileName инфор - ое сообщение
void loggingINFO(char *msg)
{
	char *head = " INFO ";
	if(initFlg) {
		FILE *file = fopen(logFileName, "a+");
		if(file != NULL) {
		

		} else {
			char *errTxt = strerror(errno);
			debugPrint(errTxt);
		}
	} else {
		char *err = "loggingINFO() - SYSTEM-ERROR: \
			     run loggingINFO() before loggingINIT() when try write:\n";
		loggingERROR(err);
		loggingERROR(msg);
	}
}

// выводим error сообщение в лог
void loggingERROR(char *msg)
{
	char *head = " ERROR ";
	printf("%s->%s", head, msg);
}

void loggingFIN()
{
	debugPrint("loggingFIN run");
	initFlg = 0;
	debugPrint("initFlg = %d\n", initFlg);
}
