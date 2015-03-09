/*
 *		Таблица маршрутов
 *
 * Version:	@(#)ort.c	1.0.0	01/09/14
 *
 * Authors:	ovelb
 *
 */

#include "ort.h"
#include "bologging.h"


/** Размер буфера для строки таблицы маршрутов */
#define RT_LSIZE 21

/** Плохой ключ */
#define RT_IKEY (char *)-1


/**
 * str_splitInt - Преобразовать входящую строку в массив типа int.
 * @buf:  Указатель на буфер с результатом.
 * @istr: Строка для разбора.
 * @dlm:  Разделитель.
 *
 * Пример: строка - "203:004:1", результат в буфере - {203, 4, 1}
 */
void str_splitInt(int *buf, const char *istr, const char *dlm)
{
	char *str = str_dup(istr);
	int i = 0;
	
	char *tok = str, *end = str;
	while (tok != NULL) {
		tok = strsep(&end, dlm);
		buf[i++] = atoi(tok);
		tok = end;
	}

	free(str);
}

/**
 * str_splitRt - Преобразовать входящую строку в структуру rtbl.
 * @rt:  Указатель на структуру.
 * @istr: Строка для разбора.
 * @dlm:  Разделитель.
 *
 */
void str_splitRt(struct rtbl *rt, const char *istr, const char *dlm)
{
	char *str = str_dup(istr);
	
	char *tok = str, *end = str;
	if (tok != NULL) {
		tok = strsep(&end, dlm);
		rt->adr = atoi(tok);
		if (tok != NULL) {
			tok = strsep(&end, dlm);
			strcpy(rt->ip, tok);
			if (tok != NULL) {
				tok = strsep(&end, dlm);
				rt->port = atoi(tok);
			}
		}
	}
	
	free(str);
}

/**
 * str_splitVal - Преобразовать значение, найденное по ключу, в структуру rtbl.
 * @rt:  Указатель на структуру.
 * @istr: Строка для разбора.
 * @dlm:  Разделитель.
 *
 */
void str_splitVal(struct rtbl *rt, const char *istr, const char *dlm)
{
	char *str = str_dup(istr);
	
	char *tok = str, *end = str;
	if (tok != NULL) {
		tok = strsep(&end, dlm);
		strcpy(rt->ip, tok);
		if (tok != NULL) {
			tok = strsep(&end, dlm);
			rt->port = atoi(tok);
		}
	}

	free(str);
}

/**
 * str_trim - Удалить управляющие символы и пробелы по краям строки.
 * @str: Строка для разбора.
 * @return  Указатель на возвращаемую строку.
 *
 */
static char *str_trim(const char *str)
{
	char *end;
	static char tstr[RT_LSIZE+1];
	
	if (str == NULL)
		return NULL;

	/** Начало строки */
	while (isspace((int)*str)) str++;

	memset(tstr, 0, RT_LSIZE+1);
	strcpy(tstr, str);

	/** Конец строки */
	end = tstr + strlen(tstr) - 1;
	while (end > tstr && isspace((int)*end)) end--;

	*(end+1) = (char)0;  /** 0 терминатор */
	return (char*)tstr;
}

/**
 * rt_free - Освободить всю память, которую занимали данные таблицы маршрутов.
 * @ht: Освобождаемая таблица.
 *
 */
void rt_free(TOHT *ht)
{
	ht_free(ht);
}

/**
 * rt_getstring - Получить значение строку по входящему ключу.
 * @ht:  Таблица для поиска.
 * @key: Ключ в виде - "adr".
 * @def: Значение по умолчанию, если ничего не нашли.
 * @return   Указатель на строку которую нашли.
 *
 */
static char *rt_getstring(TOHT *ht, const char *key, char *def)
{
	char *sval;

	if (ht == NULL || key == NULL)
		return def;

	sval = ht_get(ht, key, def);
	return sval;
}

/**
 * rt_getsize - Получить размер хэш таблицы.
 * @ht:  Хэш таблица.
 *
 * @return  ht->size.
 */
int rt_getsize(TOHT *ht)
{
	return ht->size;
}


/**
 * rt_iskey - Проверка наличия ключа в хэш таблице.
 * @ht:  Хэш таблица.
 * @key: Ключ.
 *
 * @return  1- если ключ найден, иначе - 0.
 */
int rt_iskey(TOHT *ht, const char *key)
{
	if (key == NULL) return 0;
	
	if (get_key_index(ht, key) < ht->size)
		/* Ключ найден */
		return 1;
	else
		return 0;
}

/**
 * rt_getkey - Получить ключ из хэш таблицы по индексу.
 * @ht:  Хэш таблица.
 * @idx: Индекс.
 *
 * @return  Если ключ найден получаем ключ ввиде строки, иначе - NULL.
 */
char *rt_getkey(TOHT *ht, int idx)
{
	if (idx < ht->size)
		return ht->key[idx];
	else
		return NULL;
}

/**
 * rt_getip - Получить значение IP по входящему ключу.
 * @ht:  Таблица для поиска.
 * @key: Ключ в виде - "adr".
 * @ip:  Указатель на буфер куда будет положен IP адрес.
 *
 */
void rt_getip(TOHT *ht, const char *key, char *ip)
{
	struct rtbl rt;
	char *sval;
	
	sval = rt_getstring(ht, key, "NULL:0");
	if (strstr(sval, "NULL")) {
		sval = "NULL:0";
	}
	
	str_splitVal(&rt, sval, ":");
	strcpy(ip, rt.ip);
}

/**
 * rt_getport - Получить значение порта RS485 по входящему ключу.
 * @ht:  Таблица для поиска.
 * @key: Ключ в виде - "adr".
 * @return   Найденое значение порта, 0- ключ не существует.
 *
 */
int rt_getport(TOHT *ht, const char *key)
{
	struct rtbl rt;
	char *sval;

	sval = rt_getstring(ht, key, "NULL:0");
	str_splitVal(&rt, sval, ":");
	
	return rt.port;
}

/**
 * rt_put - Занести запись в таблицу.
 * @ht:  Таблица.
 * @key: Ключ.
 * @val: Новое значение.
 * @return  0 - нет ошибки, -1 - ошибка при загрузки в хэш таблицу.
 *
 * Если ключ уже имеется в таблице, новое значение заменит старое.
 * Если ключ новый, создается новая запись.
 */
int rt_put(TOHT *ht, const char *key, const char *val)
{
	return ht_put(ht, key, val) ;
}

/**
 * rt_remove - Удалить запись в таблице.
 * @ht:  Таблица.
 * @key: Ключ для удаления записи.
 *
 */
void rt_remove(TOHT *ht, const char *key)
{
	ht_remove(ht, key);
}


/**
 * rt_save - Сохранить данные таблицы маршрутов в файл.
 * @ht:  Таблица.
 * @rtname: Имя файла таблицы маршрутов.
 *
 */
void rt_save(TOHT *ht, const char *rtname)
{
	FILE *out;
	int i;

	if (ht == NULL) return;
	if ((out = fopen(rtname, "w")) == NULL) return;
	
	if (ht->n) {
		for (i=0; i<ht->size; i++) {
			if (ht->key[i])
				fprintf(out, "%s:%s\n", ht->key[i], ht->val[i]);
		}
	}
	
	fclose(out);
}


/**
 * rt_load - Загрузка и разбор файла в таблицу.
 * @rtname: Имя файла таблицы маршрутов.
 * @return  Указатель на таблицу или NULL.
 *
 * Таблица маршрутов (adr:ip:port):
 * ---------
 * 022:192.168.1.13:2
 * 025:192.168.1.13:2
 * 140:192.168.1.13:1
 * ---------
 * что означает -
 *  ip=192.168.1.13
 *  RS485 порт 1 имеет устройство с адресом 140;
 *  RS485 порт 2 имеет устройства с адресами 22, 25.
 */
TOHT *rt_load(const char *rtname)
{
	TOHT *ht;
	FILE *in;

	struct rtbl rt;
	
	char line [RT_LSIZE+1];
	char key [4];
	char val [18];
	
	int err = 0;

	if ((in = fopen(rtname, "r")) == NULL) {
		fprintf(stderr, "rt_load: cannot open %s\n", rtname);
		return NULL;
	}

	ht = ht_new(0);
	if (!ht) {
		fclose(in);
		return NULL;
	}

        /** Инициализация буферов */
	memset(line, 0, RT_LSIZE);

	while (1) {
		/** Считываем строку и проверяем на конец файла
		    или ошибку чтения */
		if (fgets(line, sizeof(line), in) == NULL) {
			if (feof(in) != 0) {  
				/** Успешно загрузили файл */
				break;
			} else {
				/** Произошла ошибка чтения из файла */
				fprintf(stderr, "rt_load: cannot read %s\n",
					rtname);
				err = -1;
				break;
			}
		}
		/** Если файл не закончился, и не было ошибки чтения */
		strcpy(line, str_trim(line));

		if ((int)strlen(line) == 0)
			/** Отбрасываем пустые строки */
			continue;
		
		str_splitRt(&rt, line, ":");
		sprintf(key, "%03d", rt.adr);
		sprintf(val, "%s:%d", rt.ip, rt.port);
		err = ht_put(ht, key, val);
		
		if (err < 0) {
			fprintf(stderr, "rt_load: error in rt_load()\n");
			break;
		}
		/** Очистили буфер для очередной строки файла */
		memset(line, 0, RT_LSIZE);
	}

	if (err < 0) {
		ht_free(ht);
		ht = NULL;
	}
	/** Закрываем файл */
	if (fclose(in) == EOF)
		fprintf(stderr, "rt_load: cannot close %s\n", rtname);
	
	return ht;
}

