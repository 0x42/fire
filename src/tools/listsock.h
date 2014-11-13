#ifndef LISTSOCK_H
#define	LISTSOCK_H
#define BO_IP_MAXLEN 15
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "ort.h"

struct bo_sock {
	int sock;
	char ip[BO_IP_MAXLEN]; /* XXX.XXX.XXX.XXX */
        /* флаг отправки данных по этому сокету
           [1]- OK; [-1]- FALSE */
        int flag; 
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

void bo_setflag_bysock(struct bo_llsock *llist, int sock, int flag);

int bo_getip_bysock(struct bo_llsock *llist, int sock, char *ip);
#endif	/* LISTSOCK_H */

/* 0x42 */