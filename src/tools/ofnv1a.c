/*
 *		Хэш функция FNV1A
 *
 * Version:	@(#)ofnv1a.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#include "ofnv1a.h"


/**
 * mfnv1a - Вычислить хэш для строки.
 * @str: Строка символов.
 *
 * Реализация FNV1A хэш функции.
 * Дополнительно применяется операция XOR над байтами результата.
 *
 */

unsigned int mfnv1a(const char *str)
{
	unsigned int hval = 0x811c9dc5;
	unsigned int FNV_32_PRIME = 0x01000193;
	unsigned int res = 0;
	int i;
	
	while (*str)
	{
		hval ^= (unsigned int)*str++;
		hval *= FNV_32_PRIME;
	}

	for (i=0; i<4; i++)
		res ^= (hval >> i*8) & 0xFF;
	
	return res;
}

