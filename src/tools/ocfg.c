/*
 *		Парсер конфигурационных файлов
 *
 * Version:	@(#)ocfg.c	1.0.0	01/09/14
 *
 * Authors:	ovelb
 *
 */

#include "ocfg.h"

#include <ctype.h>


/** Размер буфера для строки конфигурационного файла */
#define CFG_LSIZE 512

/** Плохой ключ */
#define CFG_IKEY (char *)-1


/**
 * str_trim - Удалить управляющие символы и пробелы по краям строки.
 * @str: Строка для разбора.
 * @return  Указатель на возвращаемую строку.
 *
*/
static char *str_trim(const char *str)
{
	char *end;
	static char tstr[CFG_LSIZE+1];
	
	if (str == NULL)
		return NULL;

	/** Начало строки */
	while (isspace((int)*str)) str++;

	memset(tstr, 0, CFG_LSIZE+1);
	strcpy(tstr, str);

	/** Конец строки */
	end = tstr + strlen(tstr) - 1;
	while (end > tstr && isspace((int)*end)) end--;

	*(end+1) = (char)0;  /** 0 терминатор */
	return (char*)tstr;
}

/**
 * cfg_free - Освободить всю память, которую занимали данные
 *            конфигурационного файла.
 * @ht: Освобождаемая таблица.
 *
*/
void cfg_free(TOHT *ht)
{
	ht_free(ht);
}

/**
 * cfg_getstring - Получить значение строку по входящему ключу.
 * @ht:  Таблица для поиска.
 * @key: Ключ в виде - "section:key".
 * @def: Значение по умолчанию, если ничего не нашли.
 * @return   Указатель на строку которую нашли.
 *
*/
char *cfg_getstring(TOHT *ht, const char *key, char *def)
{
	char *sval;

	if (ht == NULL || key == NULL)
		return def;

	sval = ht_get(ht, key, def);
	return sval;
}

/**
 * cfg_getint - Получить целое значение по входящему ключу.
 * @ht:  Таблица для поиска.
 * @key: Ключ в виде - "section:key".
 * @nf:  Значение, если ничего не нашли.
 * @return  Найденое целое значение.
 *
*/
int cfg_getint(TOHT *ht, const char *key, int nf)
{
	char *str;

	str = cfg_getstring(ht, key, CFG_IKEY);
	if (str == CFG_IKEY) return nf;
	return atoi(str);
}

/**
 * cfg_put - Занести запись в таблицу.
 * @ht:  Таблица.
 * @key: Ключ.
 * @val: Новое значение.
 * @return  0 - нет ошибки, -1 - ошибка при загрузки в хэш таблицу.
 *
 * Если ключ уже имеется в таблице, новое значение заменит старое.
 * Если ключ новый, создается новая запись.
*/
int cfg_put(TOHT *ht, const char *key, const char *val)
{
	return ht_put(ht, key, val) ;
}

/**
 * cfg_remove - Удалить запись в таблице.
 * @ht:  Таблица.
 * @key: Ключ для удаления записи.
 *
*/
void cfg_remove(TOHT *ht, const char *key)
{
	ht_remove(ht, key);
}

/**
 * cfg_sections - Заполнить таблицу секций.
 * @ht:   Таблица с данными конфигурации.
 * @sect: Таблица секций.
 * @return  Указатель на таблицу секций, NULL - ошибка.
 *
*/
TOHT *cfg_sections(TOHT *ht, TOHT *sect)
{
	char s[CFG_LSIZE+1];
	char skey[CFG_LSIZE+1];
	
	int i, j;
	int slen;
	int fl;

	if (sect == NULL) return NULL;
	
	memset(s, 0, CFG_LSIZE);
	memset(skey, 0, CFG_LSIZE);

	for (i=0; i<ht->size; i++) {

		if (ht->key[i] == NULL) continue;
		if (sscanf(ht->key[i], "%[^:]", s) != 1)
			return NULL;

		/** Проверка наличия секции в таблице секций */
		slen  = (int)strlen(s);
		fl = 0;
		for (j=0; j<sect->n; j++) {
			if (!strncmp(ht->key[i], sect->val[j], slen)) {
				fl = 1;
				break;
			}
		}
		
		if (!fl) {
			/** Новая секция */
			sprintf(skey, "%04d", i);
			ht_put(sect, skey, s);
		}
	}

	return sect;
}

/**
 * cfg_save - Сохранить данные конфигурации в файл.
 * @ht:  Таблица с данными конфигурации.
 * @out: Указатель на открытый файл для сохранения данных конфигурации.
 *
*/ 
void cfg_save(TOHT *ht, FILE *out)
{
	TOHT *sect;
	int i, j;
	int slen;

	if (ht == NULL || out == NULL) return;

	sect = ht_new(0);
	
	if (cfg_sections(ht, sect) != NULL) {
	
		for (i=0; i<sect->n; i++) {
			/** Новая секция */
			fprintf(out, "\n[%s]\n", sect->val[i]);
			slen  = (int)strlen(sect->val[i]);
			
			for (j=0; j<ht->size; j++) {

				if (ht->key[j] == NULL) continue;

				if (!strncmp(ht->key[j], sect->val[i], slen)) {
					/** Если значение key=value
					 * принадлежит секции */
					fprintf(out,
						"%s = %s\n",
						ht->key[j]+slen+1,
						ht->val[j] ? ht->val[j] : "");
				}
			}
		}
	}
	
	ht_free(sect);
	
	return;
}

/**
 * cfg_parse_line - Разбор строки конфигурационного файла.
 * @cfgname: Имя конфигурационного файла.
 * @line:    Строка для разбора.
 * @lno:     Номер разбираемой строки в конфигурационном файле.
 * @sect:    Секция конфигурационного файла.
 * @return  0 - нет ошибки, -1 - ошибка при разборе
 *          или загрузки в таблицу.
 */
int cfg_parse_line(TOHT *ht,
		   const char *cfgname,
		   const char *line,
		   int lno,
		   char *sect)
{
	char key [CFG_LSIZE+1];
	char val [CFG_LSIZE+1];
	char buf [CFG_LSIZE+1];

	int len;
	int err = 0;

        /** Инициализация буферов */
	memset(key, 0, CFG_LSIZE);
	memset(val, 0, CFG_LSIZE);

	len = (int)strlen(line);

        if (line[0] == '#') {
		/** Строка комментария */
		err = 0;
	} else if (line[0] == '[' && line[len-1] == ']') {
		/** Строка с секцией */
		sscanf(line, "[%[^]]", sect);
 		err = 0;
	} else if (sscanf(line, "%[^=] = %[^#]", key, val) == 2) {
		/** Строка key=value (возможно с комментарием) */
		strcpy(key, str_trim(key));
		strcpy(val, str_trim(val));
		sprintf(buf, "%s:%s", sect, key);
		err = ht_put(ht, buf, val);
	} else {
		/** Обнаружены синтаксические ошибки в строке */
		fprintf(stderr, "cfg: syntax error in %s (%d):\n",
			cfgname, lno);
		fprintf(stderr, "-> %s\n", line);
		err = -1;
	}
	
	return err;
}


/**
 * cfg_load - Загрузка и разбор конфигурационного файла в таблицу.
 * @cfgname: Имя конфигурационного файла.
 * @return  Указатель на таблицу.
 *
*/
TOHT *cfg_load(const char *cfgname)
{
	TOHT *ht;
	FILE *in;

	char line [CFG_LSIZE+1];
	char sect [CFG_LSIZE+1];
	
	int len;
	int lno = 0;
	int err = 0;

	if ((in = fopen(cfgname, "r")) == NULL) {
		fprintf(stderr, "cfg: cannot open %s\n", cfgname);
		return NULL;
	}

	ht = ht_new(0);
	if (!ht) {
		fclose(in);
		return NULL;
	}

        /** Инициализация буферов */
	memset(line, 0, CFG_LSIZE);
	memset(sect, 0, CFG_LSIZE);

	while (1) {
		/** Считываем строку и проверяем на конец файла
		    или ошибку чтения */
		if (fgets(line, sizeof(line), in) == NULL) {
			if (feof(in) != 0) {  
				/** Успешно загрузили файл */
				break;
			} else {
				/** Произошла ошибка чтения из файла */
				fprintf(stderr, "cfg: cannot read %s\n",
					cfgname);
				err = -1;
				break;
			}
		}
		/** Если файл не закончился, и не было ошибки чтения */
		lno++;
		strcpy(line, str_trim(line));

		len = (int)strlen(line);

		if (len == 0)
			/** Отбрасываем пустые строки */
			continue;
		
		/** Длина строки файла больше размера буфера ? */
		if (len == sizeof(line)-1) {
			fprintf(stderr,	"cfg: line too long in %s (%d)\n",
				cfgname, lno);
			err = -1;
			break;
		}

		err = cfg_parse_line(ht, cfgname, line, lno, sect);
		
		if (err < 0) {
			fprintf(stderr, "cfg: error in cfg_parse_line()\n");
			break;
		}
		/** Очистили буфер для очередной строки файла */
		memset(line, 0, CFG_LSIZE);
	}

	if (err < 0) {
		ht_free(ht);
		ht = NULL;
	}
	/** Закрываем файл */
	if (fclose(in) == EOF)
		fprintf(stderr, "cfg: cannot close %s\n", cfgname);
	
	return ht;
}

