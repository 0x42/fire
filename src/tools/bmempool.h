#ifndef BMEMPOOL_H
#define	BMEMPOOL_H
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <obstack.h>
#include <errno.h>
#include <setjmp.h>
#include <mcheck.h>

struct obstack *b_crtStack();

void *b_allocInStack(struct obstack *ptr, size_t size);

void b_delStack(struct obstack *ptr);

#endif	/* BMEMPOOL_H */

/* 0x42 */