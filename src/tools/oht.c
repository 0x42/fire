/*
 *		Хэш таблица для строк
 *
 * Version:	@(#)oht.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#include "oht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/** Минимальное выделяемое количество записей в таблице */
#define HT_MINSIZE 8


/**
 * mem_reorg - Реорганизовать выделенную память под хэш таблицу.
 * @ptr:  Старый указатель на выделенную область.
 * @size: Старый размер хеш таблицы.
 * @st:   Размер типа элемента объекта.
 * @return   Указатель на выделенную область.
 *
 * Новый размер хэш таблицы будет изменен на HT_MINSIZE.
*/
static void *mem_reorg(void *ptr, int size, size_t st)
{
	void *newptr;

	newptr = calloc((size+HT_MINSIZE)*st, 1);
	if (newptr == NULL) return NULL;

	memcpy(newptr, ptr, size*st);
	free(ptr);
	return newptr;
}

/**
 * str_dup - Дублирование строки 
 * @dstr: Дубликат исходной строки
 * @return   Указатель на новую строку.
 *
 * Это аналог функции strdup().
*/
static char *str_dup(const char *str)
{
	char *dstr;
	
	if (!str) return NULL;
	dstr = (char*)malloc(strlen(str)+1);
	if (dstr) strcpy(dstr, str);
	
	return dstr;
}

/**
 * ht_hash - Вычислить хэш-ключ для строки.
 * @key: Строка символов.
 *
 * Реализация FNV1A хэш функции.
 *
 * Коллизии с хэш ключами разрешаются путем сравнения входного ключа
 * с ключем хранящимся в структуре (смотри функцию get_key_index()).
 */

static unsigned int ht_hash(const char *key)
{
	unsigned int hval = 0x811c9dc5;
	unsigned int FNV_32_PRIME = 0x01000193;

	while (*key)
	{
		hval ^= (unsigned int)*key++;
		hval *= FNV_32_PRIME;
	}

	return hval;
}


/**
 * get_key_index - Получить индекс ключа в хэш таблице.
 * @ht:  Хэш таблица.
 * @key: Ключ.
 *
 * Если ключ найден получаем индекс ключа, иначе - ht->size.
*/
static int get_key_index(TOHT *ht, const char *key)
{
	unsigned hash = ht_hash(key);
	int i;

	for (i=0; i<ht->size; i++) {
		if (ht->key[i] == NULL) continue ;
		/* Сравнение хэшей */
		if (hash == ht->hash[i]) {
			/* Сравниваем строки для разрешения коллизий */
			if (!strcmp(key, ht->key[i])) {
				return i;
			}
		}
	}
	return ht->size;
}

/**
 * modify_value - Изменение значения в хэш таблице.
 * @ht:  Хэш таблица.
 * @key: Ключ.
 * @val: Новое значение.
 *
 * Если значение существует, то изменяем его и выходим с
 * результатом 0. Иначе - выходим с результатом 1.
*/
static int modify_value(TOHT *ht, const char *key, const char *val)
{
	int i = get_key_index(ht, key);
	if (i < ht->size) {
		/* Нашли значение: изменить и выйти */
		if (ht->val[i] != NULL)
			free(ht->val[i]);
		ht->val[i] = val ? str_dup(val) : NULL;
		return 0;
	}
	return 1;
}

/**
 * add_value - Добавить значение в хэш таблицу.
 * @ht:  Хэш таблица.
 * @key: Ключ.
 * @val: Новое значение.
 *
 * Если добавление нового значения прошло успешно, то выходим с
 * результатом 0. Иначе - выходим с результатом -1.
*/
static int add_value(TOHT *ht, const char *key, const char *val)
{
	unsigned hash = ht_hash(key);
	int i;
	
	/* Если количество записей достигло максимального размера таблицы */
	if (ht->n == ht->size) {
		/* Нарастить объем таблицы на HT_MINSIZE */
		ht->val = (char **)mem_reorg(ht->val, ht->size, sizeof(char *));
		ht->key = (char **)mem_reorg(ht->key, ht->size, sizeof(char *));
		ht->hash = (unsigned int *)mem_reorg(
			ht->hash, ht->size, sizeof(unsigned));
		if (ht->val == NULL || ht->key == NULL || ht->hash == NULL)
			return -1;  /* Не удалось увеличить таблицу */

		ht->size += HT_MINSIZE;
	}

	/* Поиск первого пустого слота для ключа */	
	i = ht->n;
	if (ht->key[i] != NULL) {
		for (i=0; i<ht->size; i++) {
			if (ht->key[i] == NULL) break;
		}
	}

	ht->key[i] = str_dup(key);
	ht->val[i] = val ? str_dup(val) : NULL;
	ht->hash[i] = hash;
	ht->n++;
	return 0;
}


/**
 * ht_new - Создать новый объект таблицы.
 * @size: Начальный размер таблицы.
 *
 * Функция создает новый объект таблицы данного размера и возвращает его.
 * Если вы не знаете заранее, какое количество записей будет в таблице,
 * задайте значение size=0.
*/
TOHT *ht_new(int size)
{
	TOHT *ht;

	if (size < HT_MINSIZE)
		size = HT_MINSIZE;

	ht = (TOHT *)calloc(1, sizeof(TOHT));
	
	if (!ht) return NULL;

	ht->size = size ;
	ht->val = (char **)calloc(size, sizeof(char*));
	ht->key = (char **)calloc(size, sizeof(char*));
	ht->hash = (unsigned int *)calloc(size, sizeof(unsigned));
	return ht;
}

/**
 * ht_free - Освободить объект таблицы.
 * @ht: Хэш таблица.
 *
 * Освободить объект таблицы и всю память, связанную с ним.
*/
void ht_free(TOHT *ht)
{
	int i;

	if (ht == NULL) return;
	for (i=0; i<ht->size; i++) {
		if (ht->key[i] != NULL)	free(ht->key[i]);
		if (ht->val[i] != NULL)	free(ht->val[i]);
	}
	free(ht->val);
	free(ht->key);
	free(ht->hash);
	free(ht);
	return;
}

/**
 * ht_get - Получить значение из таблицы.
 * @ht:  Хэш таблица.
 * @key: Ключ.
 * @def: Значение по умолчанию для возврата, если ключ не найден.
 * @return  Указатель на возвращаемую строку символов.
 *
 * Функция находит ключ в таблице и возвращает указатель на его
 * значение.
 * При неудаче, возвратит указатель на значение по умолчанию.
*/
char *ht_get(TOHT *ht, const char *key, char *def)
{
	int i = get_key_index(ht, key);

	if (i < ht->size)
		return ht->val[i];
	else
		return def;
}

/**
 * ht_put - Установить значение в таблице.
 * @ht:  Хэш таблица.
 * @key: Ключ.
 * @val: Новое значение.
 * @return   0 если функция отработала нормально, -1 в противном случае.
 *
 * Если ключ находится в таблице, то новое значение заменит старое
 * (функция modify_value()).
 * Новый ключ будет добавлен в таблицу с новым значением
 * (функция add_value()).
 *
*/
int ht_put(TOHT *ht, const char *key, const char *val)
{
	if (ht == NULL || key == NULL) return -1;

	/* Искать в таблице */
	if (ht->n > 0) {
		if (modify_value(ht, key, val) == 0)
			return 0;  /* Значение было изменено:
				      выход */
	}
	
	/* Добавить новое значение */
	return add_value(ht, key, val);
}

/**
 * ht_remove - Удалить ключ в таблице.
 * @ht:  Хэш таблица.
 * @key: Ключ.
 *
 * Если ключ найден, то он удаляется из таблицы.
 */
void ht_remove(TOHT *ht, const char *key)
{
	int i;

	if (key == NULL) return;
	
	i = get_key_index(ht, key);
	if (i == ht->size)
		/* Ключ не найден */
		return;

	/* Необходимо освободить использованную память
	   и установить поля ключа и значения в NULL */
	free(ht->key[i]);
	ht->key[i] = NULL;
	if (ht->val[i] != NULL) {
		free(ht->val[i]);
		ht->val[i] = NULL;
	}
	ht->hash[i] = 0;
	ht->n--;
	return;
}

