/*
 *		Демо Лафетный ствол
 *
 * Version:	@(#)pr.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _PR_H
#define _PR_H

#include <stdio.h>
#include <pthread.h>

#include "ocs.h"


struct actx_thread_arg {
	int port;
	int tout;
	int adr;
};

pthread_mutex_t	mx_actx;

pthread_attr_t pattr;

struct thr_rx_buf rxBuf;
struct thr_tx_buf txBuf;


void gen_pr_default_cfg(char *cfile);

void scan(struct actx_thread_arg *targ, struct thr_tx_buf *b);
void probot(struct actx_thread_arg *targ, struct thr_tx_buf *b);


#endif	/* _PR_H */

