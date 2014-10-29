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

#endif	/* LISTSOCK_H */

/* 0x42 */