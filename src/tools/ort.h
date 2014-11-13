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


struct rtbl {
	int adr;
	char ip[16];
	int port;
};


void str_splitInt(int *buf, const char *istr, const char *dlm);
void str_splitRt(struct rtbl *rt, const char *istr, const char *dlm);
void str_splitVal(struct rtbl *rt, const char *istr, const char *dlm);

void rt_free(TOHT *ht);

int rt_getsize(TOHT *ht);
int rt_iskey(TOHT *ht, const char *key);
char *rt_getkey(TOHT *ht, int idx);
void rt_getip(TOHT *ht, const char *key, char *ip);
int rt_getport(TOHT *ht, const char *key);

int rt_put(TOHT *ht, const char *key, const char *val);
void rt_remove(TOHT *ht, const char *key);

void rt_save(TOHT *ht, const char *rtname);
TOHT *rt_load(const char *rtname);


#endif	/* _ORT_H */

