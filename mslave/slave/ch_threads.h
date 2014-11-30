/*
 *		Moxa-slave (threads) RS485.
 *
 * Version:	@(#)sl_threads.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _SLTHREADS_H
#define _SLTHREADS_H

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/tcp.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


void *wdt(void *arg);
void *chan1(void *arg);  /** (порт 1 - активные устройства) */
void *chan2(void *arg);  /** (порт 2 - пассивные устройства) */
void *rtbl_recv(void *arg);
void *rtbl_send(void *arg);
void *fifo_serv(void *arg);


#endif	/* _SLTHREADS_H */

