#include "bo_net.h"
/*
 * @brief		откр сокет
 * @return		возвращает socket или -1 вслучае ошибки 
 */
int initServerSock()
{
	struct sockaddr_in saddr;
	int port = 8888;
	/* PF_INET - интернет сокет IP4 
	 * SOCK_STREAM - потоковый сокет(TCP)
	 * SOCK_DGRAM  - дейтагр(UDP)
	 */
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock == -1) {
		ans = -1;
	} else {
		memset(&saddr, 0, sizeof(struct sockaddr_in));
		/*Семейство протокола ip4*/
		saddr.sin_family = AF_INET;
		/* порт */
		saddr.sin_port = htons(port);
		/* address ip4 */
		saddr.sin_addr.s_addr = INADDR_ANY;
		if(bind(sock, (struct sockaddr *) &saddr, sizeof(saddr)) == -1) {
			close(sock);
			sock = -1;
		}
	}
	return sock;
}
/* ----------------------------------------------------------------------------
 * @brief	слушаем сокет sock, как только приходит коннект возвр сокет 
 *		связ с клиентом clientfd;
 * @return	1 - ok; -1 - error
 */
int waitConnect(int sock, int *clientfd)
{
	int ans = 1;
	int queue_len = 2;
	if(listen(sock, queue_len) == -1) ans = -1;
	else {
	/// Wait and Accept connection
		*clientfd = accept(sock, NULL, NULL);
		if(*clientfd == -1) ans = -1;
	}
	return ans;
}