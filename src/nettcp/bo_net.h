#ifndef __BO_NET_H
#define	__BO_NET_H

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>

/* инициализация сокета */
int bo_initServSock(unsigned int port, char **errTxt);

/* делает сокет пассивным */
int bo_setListenSock(unsigned int sockfd, unsigned int queue_len, char **errTxt);

/* ожидание коннекта */
int bo_waitConnect(int sock, int *clientfd, char **errTxt);

#endif	/* BO_NET_H */

/* 0x42 */