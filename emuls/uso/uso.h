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
	int adr;
	int logger;
	int snmp_q;
	char *snmp_ip;
	unsigned int lline;
	unsigned int nllines;
	int cdDest;
	int cdaId;
	int cdnsId;
	int cdquLogId;
	int cdmsId;
	int cdsqId;
	int pr;
	int test_ln;
	int test_m;
	int test_nm;
	int test_msgln;
	char *test_msg;
};

pthread_mutex_t	mx_actx;

pthread_attr_t pattr;

struct thr_rx_buf rxBuf;
struct thr_tx_buf txBuf;


TOHT *cmd;


void gen_uso_default_cfg(char *cfile);

int tx(struct actx_thread_arg *targ);
int rx(struct actx_thread_arg *targ);

int scan(struct actx_thread_arg *targ, struct thr_tx_buf *b);
int uso_answer(struct actx_thread_arg *targ, struct thr_tx_buf *b);
int uso(struct actx_thread_arg *targ,
	struct thr_tx_buf *b,
	unsigned int sch,
	int dst);
int uso_quNetStat(struct actx_thread_arg *targ, struct thr_tx_buf *b);
int uso_quLog(struct actx_thread_arg *targ, struct thr_tx_buf *b,
	       unsigned int line, unsigned int nlines);
int uso_quSNMP(struct actx_thread_arg *targ, struct thr_tx_buf *b);

void *actx_485(void *arg);


#endif	/* _USO_H */

