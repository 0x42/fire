/*
 *		Таблица маршрутов
 *
 * Version:	@(#)ort.c	1.0.0	01/09/14
 *
 * Authors:	ovelb
 *
 */

#include "ort.h"


/** Размер буфера для строки файла таблицы маршрутов */
#define RT_LSIZE 10

/** Плохой ключ */
#define RT_IKEY (char *)-1


/**
 * str_split - Преобразовать входящую строку в массив типа int.
 * @buf:  Указатель на буфер с результатом.
 * @istr: Строка для разбора.
 * @dlm:  Разделитель.
 *
 * Пример: строка - "203:004:1", результат в буфере - {203, 4, 1} 
 */
static void str_split(int *buf, const char *istr, const char *dlm)
{
	char *str = strdup(istr);
	char *pch;
	int i = 0;
	
	pch = strtok(str, dlm);
	while (pch != NULL)
	{
		buf[i++] = atoi(pch);
		pch = strtok(NULL, dlm);
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
 * @key: Ключ в виде - "node:adr".
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
 * rt_getport - Получить порт RS485 по входящему ключу.
 * @ht:  Таблица для поиска.
 * @key: Ключ в виде - "node:adr".
 * @nf:  Значение, если ничего не нашли.
 * @return  Найденое значение порта.
 *
 */
int rt_getport(TOHT *ht, const char *key, int nf)
{
	char *str;

	str = rt_getstring(ht, key, RT_IKEY);
	if (str == RT_IKEY) return nf;
	return atoi(str);
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
 * @return  Указатель на таблицу.
 *
 * Таблица маршрутов (node:adr:port):
 * ---------
 * 013:022:2
 * 013:025:2
 * 013:140:1
 * ---------
 * что означает -
 *  network=192.168.0;
 *  узел=13 (ip=192.168.0.13);
 *  RS485 порт 1 имеет устройство с адресом 140;
 *  RS485 порт 2 имеет устройства с адресами 22, 25.
 */
TOHT *rt_load(const char *rtname)
{
	TOHT *ht;
	FILE *in;

	char line [RT_LSIZE+1];
	char key [8];
	char val [2];
	int buf [4];
	
	int err = 0;

	if ((in = fopen(rtname, "r")) == NULL) {
		fprintf(stderr, "rt: cannot open %s\n", rtname);
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
				fprintf(stderr, "rt: cannot read %s\n",
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
		
		str_split(buf, line, ":");
		sprintf(key, "%03d:%03d", buf[0], buf[1]);
		sprintf(val, "%d", buf[2]);
		err = ht_put(ht, key, val);
		
		if (err < 0) {
			fprintf(stderr, "rt: error in rt_load()\n");
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
		fprintf(stderr, "rt: cannot close %s\n", rtname);
	
	return ht;
}

