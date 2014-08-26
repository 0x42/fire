#include "logging.h"
// флаг проверки инициализации
static int initFlg = 0;
// имя лог файла
static char *logFileName;

// иниц-ия получение имени файла для логирования
void loggingINIT()
{
	debugPrint("loggingINIT run\n");
	initFlg = 1;
	logFileName = "log.conf";
	debugPrint("initFlg = %d\nlogFileName = %s\n", initFlg, logFileName);
}

void loggingFIN()
{
	debugPrint("loggingFIN run");
	initFlg = 0;
	debugPrint("initFlg = %d\n", initFlg);
}
