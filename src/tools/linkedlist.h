#ifndef LINKEDLIST_H
#define	LINKEDLIST_H
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
typedef struct LinkedList
{
	struct LLItem *Head;
	struct LLItem *Tail;
	size_t	      length;
} LinkedList;

typedef struct LLItem
{
	struct LLItem	*Next;
	void		*Object;
	size_t		Size;
} LLItem;

#endif	/* LINKEDLIST_H */
/* 0x42 */
