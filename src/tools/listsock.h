#ifndef LISTSOCK_H
#define	LISTSOCK_H
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct bo_sock {
	int sock;
	char ip[15]; /* XXX.XXX.XXX.XXX */
};

struct bo_llsock {
	struct bo_sock *val;
	int *prev;
	int *next;
	int *free;
	int head;
	int tail;
	int size;
	int n;
};

struct bo_llsock * bo_crtLLSock(int);

void bo_del_lsock(struct bo_llsock *);

int bo_addll(struct bo_llsock *llist, int sock);

int bo_get_val(struct bo_llsock *llist, struct bo_sock **val, int i);

void bo_print_list(struct bo_llsock *llist);

void bo_print_list_val(struct bo_llsock *ll);

int bo_get_head(struct bo_llsock *llist);

void bo_del_val(struct bo_llsock *llist, int i);

void bo_del_bysock(struct bo_llsock *llist, int sock);

#endif	/* LISTSOCK_H */

/* 0x42 */