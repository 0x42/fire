#ifndef BO_UDP_H
#define	BO_UDP_H
#include <stdio.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <errno.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>

#include "../log/bologging.h"

/* ----------------------------------------------------------------------------
 * @brief	создаем UDP сокет
 * @return	[0] ERROR [>0] socket
 */
int bo_udp_sock();

/* Неприсоединенный сокет */
int bo_send_udp_packet(int socket, unsigned char *packet, int len, char *ip);

int bo_recv_udp_packet(
        void *buf, 
        int bufSize, 
        int socket, 
        char **sender_host, 
        int *sender_port);

#endif	/* BO_UDP_H */

/* 0x42 */