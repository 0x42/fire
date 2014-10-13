#ifndef __BO_NET_H
#define	__BO_NET_H

#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include "../tools/dbgout.h"
#include "../log/bologging.h"

/* инициализация сокета */
int bo_initServSock(unsigned int port, char **errTxt);

/* делает сокет пассивным */
int bo_setListenSock(unsigned int sockfd, unsigned int queue_len, char **errTxt);

/* ожидание коннекта */
int bo_waitConnect(int sock, int *clientfd, char **errTxt);

int bo_crtSock(char *ip, unsigned int port, struct sockaddr_in *addr);
/* отправка данных в FIFO */
int bo_sendDataFIFO(char *ip, unsigned int port, char *data, unsigned int size);

int bo_sendAllData(int sock, unsigned char *buf, int len);
/* получение данных */
 int bo_recvAllData(int sock, char *buf, int size);
#endif	/* BO_NET_H */

/* 0x42 */