/*
 *		Таблица маршрутов
 *
 * Version:	@(#)ort.h	1.0.0	01/09/14
 *
 * Authors:	ovelb
 *
 */

#ifndef _ORT_H
#define _ORT_H

#include "oht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


void rt_free(TOHT *ht);

int rt_getport(TOHT *ht, const char *key, int port);

int rt_put(TOHT *ht, const char *key, const char *val);
void rt_remove(TOHT *ht, const char *key);

void rt_save(TOHT *ht, const char *rtname);
TOHT *rt_load(const char *rtname);


#endif	/* _ORT_H */

