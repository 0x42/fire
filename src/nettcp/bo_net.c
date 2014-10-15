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

int bo_listen(int sock, int queue_len)
{
	int ans = 1;
	ans = listen(sock, queue_len);
	return ans;
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
/* ----------------------------------------------------------------------------
 * @brief	устанав соед с узлом -> отпр SET|LEN|DATA -> ждем ответ OK ->
 *		закр сокет
 * @return	[-1] - error; [1] - OK  
 */
int bo_sendDataFIFO(char *ip, unsigned int port, 
	char *data, unsigned int dataSize)
{
	struct sockaddr_in saddr;
	struct timeval tval;
	int ans  = -1;
	int sock = -1;
	int exec = -1;
	char *head = "SET";
	unsigned char len[2] = {0};
	char buf[4] = {0};
	char *ok = NULL;
	sock = bo_crtSock(ip, port, &saddr);
	/* 100 мсек*/
	tval.tv_sec = 0;
	tval.tv_usec = 100000;
	if(sock != -1) {
		/* устан максимальное время ожидания одного пакета */
		setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(tval));
		if(connect(sock, 
			  (struct sockaddr *) &saddr, 
			   sizeof(struct sockaddr)) == 0) {
			boIntToChar(dataSize, len);
			exec = bo_sendAllData(sock, (unsigned char*)head, 3);
			if(exec == -1) goto error;
			exec = bo_sendAllData(sock, len, 2);
			if(exec == -1) goto error;
			exec = bo_sendAllData(sock, (unsigned char*)data, dataSize);
			if(exec == -1) goto error;
			exec = bo_recvAllData(sock, (unsigned char*)buf, 3, 3);
			if(exec == -1) goto error;
			else {
				ok = strstr(buf, "OK");
				if(ok) ans = 1;
				else {
					bo_log("bo_sendDataFIFO() wait[OK] but recv[%s]", buf);
				}
			}
		} else {
			error:
			bo_log("bo_sendDataFIFO() errno[%s]\n ip[%s]\nport[%d]\nsize[%d]", 
			strerror(errno),
			ip,
			port,
			dataSize);
		}
		close(sock);
	} else {
		bo_log("bo_sendDataFIFO() errno[%s]\n ip[%s]\nport[%d]\nsize[%d]", 
			strerror(errno),
			ip,
			port,
			dataSize);
	}
	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief	созд сокет 
 */
int bo_crtSock(char *ip, unsigned int port, struct sockaddr_in *saddr)
{
	int sock = -1;
	struct in_addr servip;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock != -1) {
		saddr->sin_family = AF_INET;
		saddr->sin_port = htons(8888);
		inet_aton(ip, &servip);
		saddr->sin_addr.s_addr = servip.s_addr;
	};
	return sock;
}
 /* ---------------------------------------------------------------------------
  * @brief		отправляет данные 
  * @buf		данные которые будут отправлены
  * @len		размер отправл данных
  * @return		[-1] - ERROR [>0] - кол отпр байт
  */
 int bo_sendAllData(int sock, unsigned char *buf, int len)
 {
	int count = 0;   /* кол-во отпр байт за один send*/
	int allSend = 0; /* кол-во всего отправл байт*/
	unsigned char *ptr = buf;
	int n = len; 
	while(allSend < len) {
		count = send(sock, ptr + allSend, n - allSend, 0);
		if(count == -1) break;
		allSend += count;
	}
	return (count == -1 ? -1 : allSend);
 }
 /* ---------------------------------------------------------------------------
  * @brief		получаем данные размера length
  * @return		[-1] - ERROR [count] - кол во пол данных	
  */
int bo_recvAllData(int sock, unsigned char *buf, int bufSize, int length)
{
	int count = 0;
	int exec = 1;
	int all = 0;
	while(all < length) {
		count = recv(sock, buf + all, bufSize - all, 0);
		if(count < 1) { 
			if(all != length) exec = -1;
			break;
		}
		all += count;
	}
	return ( exec == -1 ? -1 : all);
}