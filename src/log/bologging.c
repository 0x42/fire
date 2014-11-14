#include "bologging.h"
/* -------------------------------------------------------------------------- */

STATIC void sysErr(char *msg, ...);
STATIC void sysErrParam(char *msg, va_list *ap);
STATIC void bo_setLogParam(char *fname, char *oldfname, int nrow, int maxrow);
STATIC int wrtLog(char *msg, va_list *ap, char **errTxt);
STATIC int log_fprintf(FILE *f, char *timeBuf, va_list *ap, char *msg);
STATIC int readNRow(const char *fname);
STATIC int delOldFile(char *fname);

/* ---------------------------------------------------------------------------- 
 */
static struct {
	/*pid процесса*/
	int pid;
	/* имя лог файла*/
	char *name;
	char *oldname;
	/*текущ. кол-во строк в лог файле*/
	int nrow;
	/*макс кол-во строк в лог файле*/
	int maxrow;
} log = {0};

/* инициализация MUTEX*/
static pthread_mutex_t bolog_mutex = PTHREAD_MUTEX_INITIALIZER;

/* -1 struct log - не иниц-на; 1 - инициал-на*/
static int logInit = -1;
/* ----------------------------------------------------------------------------
 * @brief		иниц struct log
 * @param fname		назв тек лог файла
 * @param oldfname	старый лог файл
 * @param nrow		текущ кол-во строк
 * @param maxrow	макс кол-во строк
 */
void bo_setLogParam(char *fname, char *oldfname, int nrow, int maxrow)
{
	log.pid = getpid();
	log.name = fname;
        log.oldname = oldfname;
	log.nrow = nrow;
	log.maxrow = maxrow;
	logInit = 1;
}
/* ----------------------------------------------------------------------------
 * @brief		сбрасываем флаг logInit
 */
void bo_resetLogInit()
{
	logInit = -1;
}
/* ----------------------------------------------------------------------------
 * @brief		иниц-ия struct log получение имени файла для логирования
 */

void loggingINIT()
{
	/* получ-ие pid процесса*/
	log.pid = getpid();
	/* ЧИТАТЬ С КОНФ ФАЙЛА*/
	log.name = "default.log";
	log.oldname = "default_OLD.log";
	log.maxrow = 1000;
	/* ================= */
        logInit = 1;
	log.nrow = readNRow(log.name);
}
/* ----------------------------------------------------------------------------
 * @brief		выводим в logFileName инф-ое сообщение
 *			%d - int %f - float %s - string
 * @param msg		инфор которая вывод в лог	
 * @return		возвр -1 в случае ошибки, >0 вслучае успешной записи
 */
int bo_log(char *msg, ...)
{
	char *errTxt = NULL;
	int err = 0, ans = 0;
	/* указывает на очередной безымян-ый аргумент */
	va_list ap; 
	
	pthread_mutex_lock(&bolog_mutex);
	
	/* защита если msg = null */
	if(msg == NULL) msg = " ";
	if(strlen(msg) < 1) msg = " ";
	
	/* устнав на 1 безым аргумент */
	va_start(ap, msg);
	if(logInit < 0) loggingINIT();
	if(logInit) {
		if(wrtLog(msg, &ap, &errTxt) < 0) err = 1;
	} else {
		errTxt = "bo_log() can't run loggingINIT()";
		err = 1;
	}
	if(err) {
		sysErr("bo_log() errno[%s]\n msg:\n", errTxt);
		sysErrParam(msg, &ap);
		ans = -1;
	} else ans = 1;
	/* очистка */
	va_end(ap);
	
	pthread_mutex_unlock(&bolog_mutex);
	
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	пишим в лог файл(указаный в log.name)
 * @msg		текст с параметрами
 * @ap		указатель на параметры 
 * @errTxt	если произошла ошибка возвращает текст errno в errTxt
 *		освобождать не надо тк результат из errno 
 * @return	>0 = ok; <0 = error 
 */


STATIC int wrtLog(char *msg, va_list *ap, char **errTxt)
{
	int ans = 1;
	char timeBuf[40] = {0};
	FILE *file = NULL;
	/* проверка чтобы в логе сохр послед 1000 записей */
	if(bo_isBigLogSize(&log.nrow, log.maxrow, log.name, log.oldname) > 0) {
		file = fopen(log.name, "a+");
		bo_getTimeNow(timeBuf, sizeof(timeBuf));
		if(file) {
			if(log_fprintf(file, timeBuf, ap, msg) < 0) {
				*errTxt = strerror(errno);
				ans = -1;
			} else log.nrow++;

			if(fclose(file) < 0) {
				*errTxt = strerror(errno);
				ans = -1;
			}
		} else {
			*errTxt = strerror(errno);
			ans = -1;
		}
	} else {
		ans = -1;
	}
	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief		вывод в файл
 * @param timeBuf	время
 * @param ap		указ на первый безымяный параметр
 * @param msg		сообщение возм испол %s--строка %d-число %f - double 
 * @return		-1 error; >0 - ok
 */
 int log_fprintf(FILE *f,  char *timeBuf, va_list *ap, char *msg)
{
	int ans = 1;
	char *p = 0, *sval = 0;
	int ival = 0; 
	double dval;
	if(log.pid == 0) log.pid = getpid();
	if(fprintf(f, "%s", timeBuf) < 0) return -1;
	if(fprintf(f, " [%d] ", log.pid) < 0) return -1;
	for(p = msg; *p; p++) {
		if(*p != '%') {
			if(fprintf(f, "%c", *p) < 0) return -1;
			continue;
		}
		switch(*++p) {
			case 'd':
				ival = va_arg(*ap, int);
				if(fprintf(f, "%d", ival) < 0) return -1;
				break;
			case 'f':
				dval = va_arg(*ap, double);
				if(fprintf(f, "%f", dval) < 0) return -1;
				break;
			case 's':
				sval = va_arg(*ap, char *);
				if(fprintf(f, "%s", sval) < 0) return -1;
				break;
			default:
				if(fprintf(f, "%c", *p) < 0) return -1;
				break;
		}
	}
	if(fprintf(f, "\n") < 0) return -1;
	return ans;
}
/* ----------------------------------------------------------------------------
 * @return	возвр. кол-во строк в fname файле ecли файл не сущест.
 *		или не удалось открыть возв 0
 */

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
		fclose(file);
	}
	return nrow;
}

/* ---------------------------------------------------------------------------- 
 * @brief		В moxa нет systemd(сохраняем инфор о ошибках в системный 
 *			лог /var/log/syslog) Пишим в /dev/log
 * @param funcName	имя функции в которой произошла ошибка
 * @param msg		причина ошибки 
 */
 void sysErr(char *msg, ...) 
{
	FILE *file = NULL;
	char timeBuf[40] = {0};
	va_list ap;
	bo_getTimeNow(timeBuf, sizeof(timeBuf));
	
	file = fopen(SYSERRFILE, "a+");
	if(file) {
		va_start(ap, msg);
		if(log_fprintf(file, timeBuf, &ap, msg) < 0)
			printf("Error when write in /dev/log %s\n",
				strerror(errno));
		if(fclose(file) < 0) {
			printf("Error when close /dev/log %s\n", 
				strerror(errno));
		}
		va_end(ap);
	} else {
		printf("%s%s\n", "ERROR sysErr() ", strerror(errno));
	}
}

 
void sysErrParam(char *msg, va_list *ap)
{
	FILE *file = NULL;
	char timeBuf[40] = {0};
	file = fopen(SYSERRFILE, "a+");
	bo_getTimeNow(timeBuf, sizeof(timeBuf));
	if(file) {
		if(log_fprintf(file, timeBuf, ap, msg) < 0)
			printf("Error when write in /dev/log %s\n",
				strerror(errno));
		if(fclose(file) < 0) {
			printf("Error when close /dev/log %s\n", 
				strerror(errno));
		}
	} else {
		printf("%s%s\n", "ERROR sysErr() ", strerror(errno));
	}
}
/* ----------------------------------------------------------------------------
 * @brief	 проверяем размер файла log.name если nrow = maxrow то log.name
 *		 сохраняем c именем log.oldname. Создаем пустой файл c 
 *		 именем log.name  + сбрасываем счетчик кол-во строк nrow
 */
int bo_isBigLogSize(int *nrow, int maxrow, char *name, char *oldname)
{
	int ans = 1;
	char *errnoTxt = NULL;
	if(*nrow >= maxrow) {
		if(delOldFile(oldname) > 0 ) { 
			if(rename(name, oldname) != 0) {
				errnoTxt = strerror(errno);
				sysErr("isBigLogSize() errno[%s]\n name=%s"\
					" \noldname= %s\n nrow = %d\n maxrow = %d\n", 
					errnoTxt, name,	oldname, *nrow, maxrow);
				ans = -1;
			} else *nrow = 0;
		} else {
			ans = -1;
		}
	}
	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief		удаляем файл fname если сущ-ет
 * @param fname		имя файла для удаления
 * @return		-1 error; 1 ок
 */
int delOldFile(char *fname)
{
	int err = -1;
	char *errTxt = NULL;
	int ans = 1;
	FILE *file = fopen(fname, "r");
	if(file != NULL) {
		if(fclose(file) < 0) {
			err = 1;
			errTxt = strerror(errno);
		} else {
			if(remove(fname) < 0) {
				err = 1;
				errTxt = strerror(errno);
			} 
		}
	}
	if(err > 0) {
		sysErr("delOldFile() errno[%s]\n fname=%s\n", 
			errTxt, fname);
		ans = -1;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief		возвращает дату и время в заданном формате 
 *			день-месяц-год час:мин:сек
 * @param timeStr	строка в кот-ую пишим время
 * @param sizeBuf	размер timeStr  
 */
void bo_getTimeNow(char *timeStr, int sizeBuf) 
{
	struct tm *ptr = NULL;
	time_t lt;
	int micro = 0;
	struct timeval tval;
	char buffer[30];
	
	if(sizeBuf < 40) {
		/*не пишем лог тк ошибка возможна только на этапе кодирования*/
		printf("bo_getTimeNow() - ERROR - массив не достаточного размера\n");
		return;
	}
	/*возвр текущ время системы*/
	if(gettimeofday(&tval, NULL) == -1) {
		lt = time(NULL);
	} else {
		lt = tval.tv_sec;
		micro = tval.tv_usec;
	}

	/*преобразует в структуру tm 
	 *localtime() <- mtrace() выдает memory not freed ? <-0x42*/
	ptr = localtime(&lt);
	if(strftime(buffer, sizeBuf, "%d-%m-%Y %H:%M:%S ", ptr) == 0)
		/*не пишем лог тк ошибка возможна только на этапе кодирования*/
		printf("bo_getTimeNow() - ERROR - массив не достаточного размера\n");
	sprintf(timeStr, "%s%d ", buffer, micro);
}
/* [0x42] */