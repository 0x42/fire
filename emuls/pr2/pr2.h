/*
 *		Демо Лафетный ствол 2 шт.
 *
 * Version:	@(#)pr2.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _PR2_H
#define _PR2_H

#include <stdio.h>
#include <pthread.h>

#include "ocs.h"


struct actx_thread_arg {
	int port;
	int tout;
	int adr1;
	int adr2;
};

pthread_mutex_t	mx_actx;

pthread_attr_t pattr;

struct thr_rx_buf rxBuf;
struct thr_tx_buf txBuf;


void gen_pr2_default_cfg(char *cfile);

void scan(struct actx_thread_arg *targ, struct thr_tx_buf *b);
void probot(struct actx_thread_arg *targ, struct thr_tx_buf *b);


#endif	/* _PR2_H */

