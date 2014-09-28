#include "bo_net.h"
/*
 * @brief		откр сокет
 * @return		возвращает socket или -1 вслучае ошибки 
 */
int bo_initServSock(unsigned int port, char **errTxt)
{
	struct sockaddr_in saddr;
	/* AF_INET - интернет сокет IP4 
	 * SOCK_STREAM - потоковый сокет(TCP)
	 * SOCK_DGRAM  - дейтагр(UDP)
	 */
	int i = 1;
	int sock;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock != -1) {
		memset(&saddr, 0, sizeof(struct sockaddr_in));
		/*Семейство протокола ip4*/
		saddr.sin_family = AF_INET;
		/* порт */
		saddr.sin_port = htons(port);
		/* address ip4 */
		saddr.sin_addr.s_addr = INADDR_ANY;
		/* Позволяет ядру повторно использовать адрес сокета.
		 * Можно запускать программу два раза подряд. Не ожидая пока 
		 * истечет ограничение на повторное испол кортежа (ip, port)(2min)
		 * SOL_SOCKET - указывает на установку опции обобщеного сокета
		 * SO_REUSEADDR - опция которая подлежит изменению
		 * &i - указатель на новое значение для опции
		 */
		i = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
		if(bind(sock, (struct sockaddr *) &saddr, sizeof(saddr)) == -1) {
			close(sock);
			*errTxt = strerror(errno);
			sock = -1;
		}
		
	} else {
		*errTxt = strerror(errno);
	}
	return sock;
}
/* ----------------------------------------------------------------------------
 * @brief	делает сокет пассивным
 * @sockfd	socket
 * @queue_len	размер очереди необр запросов
 * @return	[1] = OK; [-1] = ERROR
 */
int bo_setListenSock(unsigned int sockfd, unsigned int queue_len, char **errTxt)
{
	int ans = 1;
	if(listen(sockfd, queue_len) == -1) {
		ans = -1;
		*errTxt = strerror(errno);
	}
	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief	слушаем сокет sock, как только приходит коннект возвр сокет 
 *		связ с клиентом clientfd;
 * @return	[1] - ok; [-1] - error
 */
int bo_waitConnect(int sock, int *clientfd, char **errTxt)
{
	int ans = 1;
	*clientfd = accept(sock, NULL, NULL);
	if(*clientfd == -1) {
		ans = -1;
		*errTxt = strerror(errno);
	}
	return ans;
}