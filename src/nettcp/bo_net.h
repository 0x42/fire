#ifndef BO_NET_H
#define	BO_NET_H

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

/* инициализация сокета*/
int initServerSock();

/* ожидание коннекта*/
int waitConnect(int sock, int *clientfd);

#endif	/* BO_NET_H */

/* 0x42 */