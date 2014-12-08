/*
 *		Демо Лафетный ствол 2 шт.
 *
 * Version:	@(#)pr.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _PR_H
#define _PR_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "ocs.h"


struct actx_thread_arg {
	int port;
	int tout;
	int adr1;
	int adr2;
	int uso1;
	int uso2;
	int uso3;
	int uso4;
};

pthread_mutex_t	mx_actx;

pthread_attr_t pattr;

struct thr_rx_buf rxBuf;
struct thr_tx_buf txBuf;


void gen_pr_default_cfg(char *cfile);

void scan(struct actx_thread_arg *targ, struct thr_tx_buf *b);
void probot(struct actx_thread_arg *targ, struct thr_tx_buf *b);

void *actx_485(void *arg);


#endif	/* _PR_H */

