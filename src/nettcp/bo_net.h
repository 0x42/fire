#ifndef __BO_NET_H
#define	__BO_NET_H

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

int bo_initServerSock();

int bo_listen(int sock, int queue_len);

int bo_waitConnect(int sock, int *clientfd);

#endif	/* BO_NET_H */

/* 0x42 */