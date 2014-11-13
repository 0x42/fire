/*
 *		Хэш таблица для строк
 *
 * Version:	@(#)oht.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _OHT_H
#define _OHT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


/**
 * хэш-таблица
 *
*/
typedef struct _oht_ {
	int n;           /** Количество записей в таблице */
	int size;        /** Размер таблицы (в записях) */
	char **val;      /** Список строковых значений */
	char **key;      /** Список строковых ключей */
	unsigned *hash;  /** Список хэш-значений для ключей */
} TOHT;


char *str_dup(const char *str);

TOHT *ht_new(int size);
void ht_free(TOHT *ht);

int ht_get_size(TOHT *ht);
int get_key_index(TOHT *ht, const char *key);
char *ht_get_key(TOHT *ht, int idx);

char *ht_get(TOHT *ht, const char *key, char *def);
int ht_put(TOHT *ht, const char *key, const char *val);
void ht_remove(TOHT *ht, const char *key);

#endif	/* _OHT_H */

