/*
 *		Парсер конфигурационных файлов
 *
 * Version:	@(#)ocfg.h	1.0.0	01/09/14
 *
 * Authors:	ovelb
 *
 */

#ifndef _OCFG_H
#define _OCFG_H

#include "oht.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


void cfg_free(TOHT *ht);

char *cfg_getstring(TOHT *ht, const char *key, char *def);
int cfg_getint(TOHT *ht, const char *key, int nf);

int cfg_put(TOHT *ht, const char *key, const char *val);
void cfg_remove(TOHT *ht, const char *key);

void cfg_save(TOHT *ht, FILE *out);
int cfg_parse_line(TOHT *ht,
		   const char *cfgname,
		   const char *line,
		   int lno,
		   char *sect);
TOHT *cfg_load(const char *cfgname);

void inc_cron_life(char *lifile);


#endif	/* _OCFG_H */

