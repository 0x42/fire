/*
 *		Демо УСО
 *
 * Version:	@(#)uso.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _USO_H
#define _USO_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#include "ocfg.h"
#include "ocs.h"


struct actx_thread_arg {
	int port;
	int tout;
	int adr1;
	int logger1;
	int adr2;
	int logger2;
	unsigned int lline;
	unsigned int nllines;
	int test_ln;
	int cdaId;
	int cdaDest;
	int cdnsId;
	int cdnsDest;
	int cdquLogId;
	int cdquDest;
	int pr1;
	int pr2;
	int pr3;
	int pr4;
};

pthread_mutex_t	mx_actx;

pthread_attr_t pattr;

struct thr_rx_buf rxBuf;
struct thr_tx_buf txBuf;


TOHT *cmd;


void gen_uso_default_cfg(char *cfile);

void scan(struct actx_thread_arg *targ, struct thr_tx_buf *b);
unsigned int uso(struct actx_thread_arg *targ, struct thr_tx_buf *b,
		 unsigned int sch, int dst);
void uso_answer(struct actx_thread_arg *targ, struct thr_tx_buf *b);
void uso_answer_error(struct actx_thread_arg *targ, struct thr_tx_buf *b);
void uso_quNetStat(struct actx_thread_arg *targ, struct thr_tx_buf *b);
void uso_quLog(struct actx_thread_arg *targ, struct thr_tx_buf *b,
	       unsigned int line, unsigned int nlines);

void *actx_485(void *arg);


#endif	/* _USO_H */

