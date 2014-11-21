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

/* запуск сервера */
int bo_servStart(int port, int queue_len);

/* ожидание коннекта */
int bo_waitConnect(int sock, int *clientfd, char **errTxt);

/* созд сокета */
int bo_crtSock(char *ip, unsigned int port, struct sockaddr_in *addr);

/* установка соединения*/
int bo_setConnect(char *ip, int port);

/* отправка данных в FIFO */
int bo_sendDataFIFO(char *ip, unsigned int port, char *data, unsigned int size);

int bo_sendSetMsg(int sock, char *data, unsigned int dataSize);
int bo_sendTabMsg(int sock, char *data, unsigned int dataSize);
int bo_sendLogMsg(int sock, char *data, unsigned int dataSize);
int bo_sendRloMsg(int sock, int index);
int bo_sendXXXMsg(int sock, char *head, char *data, int dataSize);

/* получить данные в FIFO */
int bo_recvDataFIFO(char *ip, unsigned int port, unsigned char *buf, int bufSize);

/* получ данные из таблицы маршрут по адрессу 485*/
int bo_recvRoute(char *ip, 
		 unsigned int port, 
		 char *addr485, 
	         char *buf, 
	         int bufSize);

/* отправление данных */
int bo_sendAllData(int sock, unsigned char *buf, int len);

/* получение данных */
int bo_recvAllData(int sock, unsigned char *buf, int bufSize, int len);

/* ---------------------------------------------------------------------------
  * @brief	читаем длину пакета
  * @return	[-1] - ошибка, [>0] - длина сообщения
  */
unsigned int bo_readPacketLength(int sock);

/* врем в течение кот ждем данные */
void bo_setTimerRcv(int sock);

void bo_setTimerRcv2(int sock, int sec, int mil);

/* закрытие сокета */
void bo_closeSocket(int sock);

#endif	/* BO_NET_H */

/* 0x42 */