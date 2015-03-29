#include "bo_net.h"
#include "bo_send_lst.h"

static int bo_sendData(int sock, char *data, unsigned int dataSize);
static int bo_sendData2(int sock, char *data, unsigned int dataSize);

static void delSockLst(struct BO_SOCK_LST *lst);
static int bo_addToSockLst(struct BO_SOCK_LST *lst, char *ip);
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
 * @brief	запуск слуш сокета. Ошибки пишет в лог.
 * @port	порт кот слушаем
 * @queue_len	размер очереди необр запросов
 * @return	[-1] error; [sockfd] - socket 
 */
int bo_servStart(int port, int queue_len)
{
	int ans = -1;
	int sockfd = 0;
	char *errTxt = NULL;
	sockfd = bo_initServSock(port, &errTxt);
	if(sockfd > 0) {
		if(bo_setListenSock(sockfd, queue_len, &errTxt) != 1) {
			bo_log("%s%s errno[%s]", " ERROR ", 
				"bo_ServStart() ", errTxt);
			close(sockfd);
			ans = -1;
		} else {
			ans = sockfd;
		}
	} else {
		bo_log("%s%s errno[%s]", " ERROR ", 
			"bo_ServStart()->bo_initServSock()", errTxt);
		ans = -1;
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
 * @brief	 ищет по ip нужный socket в списке; если данные не удалось отправить
 *		 сокет закрывается и пересоздается попытка повторяется 
 * @return	[-1] - error [1]- OK
 */
int bo_sendDataFIFO(char *ip, unsigned int port, 
	char *data, unsigned int dataSize)
{
	static struct BO_SOCK_LST *lst = NULL;
	static int err_count = 0;
	int ans = -1, sock = 0, exec = -1;
	
	if(lst == NULL) lst = bo_init_sock_lst(10, port);
	if(lst == NULL) goto exit;
	/**/
	sock = bo_get_sock_by_ip(lst, ip);
	if(sock  == -1) {
		sock = bo_addToSockLst(lst, ip);
		bo_log("bo_sendDataFIFO ADD CLIENT sock[%d]ip[%s]\n", sock, ip);
		if(sock == -1) {
			err_count++;
			bo_log("bo_sendDataFIFO[%s] can't add to lst", ip);
			goto exit;
		}
	}
	/* Отправка данных */
	exec = bo_sendData2(sock, data, dataSize);
	if(exec == -1) {
		bo_log("bo_sendDataFIFO->bo_sendData2[%s] can't send. Reconnect", ip);
		bo_del_by_sck_sock_lst(lst, sock);
		sock = bo_addToSockLst(lst, ip);
		bo_log("bo_sendDataFIFO ADD CLIENT sock[%d]ip[%s]\n", sock, ip);
		if(sock == -1) {
			bo_log("bo_sendDataFIFO[%s] can't add to lst", ip);
			err_count++;
			goto exit;
		}
		exec = bo_sendData2(sock, data, dataSize);
		if(exec == -1) {
			bo_log("bo_sendDataFIFO[%s] can't send", ip);
			bo_del_by_sck_sock_lst(lst, sock);
		}
		
	}
	ans = exec;
	
	if(err_count == 10) {
		err_count = 0;
		bo_log("bo_sendDataFIFO ERROR err_count = 10 delete lst");
		delSockLst(lst);
		lst = NULL;
	}
	
	exit:
	return ans;
}

static void delSockLst(struct BO_SOCK_LST *lst)
{
	int i = -1;

	i = lst->head;
	while(i != -1) {
		bo_del_item_sock_lst(lst, i);
		i = *(lst->next + i);
	}
	bo_del_sock_lst(lst);
}
/*
 * @return [sock] OK [-1] ERROR
 */
static int bo_addToSockLst(struct BO_SOCK_LST *lst, char *ip)
{
	int ans = -1, exec = -1;
	
	exec = bo_add_sock_lst(lst, ip);
	if(exec == -1) {
		bo_log("bo_addToSockLst ip[%s] ERROR can't add to lst", ip);
		goto exit;
	}
	
	exec = bo_get_sock_by_ip(lst, ip);
	if(exec == -1) {
		bo_log("bo_addToSockLst ip[%s] ERROR can't get sock after add", ip);
		goto exit;
	}
	
	ans = exec;
	exit:
	return ans;
}
/* ----------------------------------------------------------------------------
 * @brief	отправка данных 
 * @return	[-1] ERROR [1] - OK
 */
static int bo_sendData(int sock, char *data, unsigned int dataSize) 
{
	int exec = -1, ans = -1;
	const int err = -1;
	char head[3] = "SET";
	unsigned char len[2] = {0};
	char buf[4] = {0};
	if(sock != -1) {
		boIntToChar(dataSize, len);
		exec = bo_sendAllData(sock, (unsigned char*)head, 3);
		if(exec == -1) {
			bo_log("SEND HEAD ERROR errno[%s]", strerror(errno));
			goto error;
		}
		exec = bo_sendAllData(sock, len, 2);
		if(exec == -1) {
			bo_log("SEND LEN ERROR errno[%s]", strerror(errno));
			goto error;
		}
		
		exec = bo_sendAllData(sock, (unsigned char*)data, dataSize);
		if(exec == -1) {
			bo_log("SEND DATA ERROR errno[%s]", strerror(errno));
			goto error;
		}
		exec = bo_recvAllData(sock, (unsigned char*)buf, 3, 3);
		if(exec == -1) {
			bo_log("RECV ANS ERROR errno[%s]", strerror(errno));
			goto error;
		}
		if(strstr(buf, "OK")) ans = 1;
		else {
			buf[3] = 0;
			bo_log("bo_sendData wait OK recv[%s]", buf);
			goto error;
		}
	} 
	if(err == 1) {
		error:
		bo_log("bo_sendData ERROR errno[%s]", strerror(errno));
		ans = -1;
	}
	return ans;
}

static int bo_sendData2(int sock, char *data, unsigned int dataSize)
{
	int exec = -1, ans = -1;
	const int err = -1;
	char head[3] = "SET";
	unsigned char len[2] = {0};
	unsigned char tmp[1300] = {0};
	char buf[4] = {0};
	if(sock != -1) {
		memcpy(tmp, head, 3);
		boIntToChar(dataSize, len);
		memcpy(tmp + 3, len, 2);
		memcpy(tmp + 5, data, dataSize);
		
		exec = bo_sendAllData(sock, tmp, dataSize+5);
		if(exec == -1) {
			bo_log("SEND DATA ERROR errno[%s]", strerror(errno));
			goto error;
		}
		exec = bo_recvAllData(sock, (unsigned char*)buf, 3, 3);
		if(exec == -1) {
			bo_log("RECV ANS ERROR errno[%s]", strerror(errno));
			goto error;
		}
		if(strstr(buf, "OK")) { 
			ans = 1;
		} else {
			buf[3] = 0;
			bo_log("bo_sendData2 wait OK recv[%s]", buf);
			goto error;
		}
	} 
	if(err == 1) {
		error:
		bo_log("bo_sendData2 ERROR errno[%s]", strerror(errno));
		ans = -1;
	}
	return ans;
} 
/* ----------------------------------------------------------------------------
 * @brief	устанав соед с узлом -> отпр SET|LEN|DATA -> ждем ответ OK ->
 *		закр сокет
 * @return	[-1] - error; [1] - OK  
 */
int bo_sendDataFIFO_depricated(char *ip, unsigned int port, 
	char *data, unsigned int dataSize)
{
	int ans  = -1;
	int sock = -1;
	int exec = -1;
	char *head = "SET";
	unsigned char len[2] = {0};
	char buf[4] = {0};
	char *ok = NULL;
	/*
	bo_log("bo_sendDataFIFO()->bo_setConnect[%s][%d] size[%d]", ip, port, dataSize);
	*/
	 sock = bo_setConnect(ip, port);

	if(sock != -1) {
		boIntToChar(dataSize, len);
	/*	bo_log("TCP HEAD "); */
		exec = bo_sendAllData(sock, (unsigned char*)head, 3);
	/*	bo_log("TCP LEN"); */
		if(exec == -1) goto error;
		exec = bo_sendAllData(sock, len, 2);
		if(exec == -1) goto error;
	/*	bo_log("TCP DATA"); */
		exec = bo_sendAllData(sock, (unsigned char*)data, dataSize);
		if(exec == -1) goto error;
		exec = bo_recvAllData(sock, (unsigned char*)buf, 3, 3);
		if(exec == -1) {
error:
			bo_log("bo_sendDataFIFO() errno[%s]\n ip[%s]\nport[%d]\n", 
				strerror(errno), ip, port);
		} else {
			ok = strstr(buf, "OK");
			if(ok) ans = 1;
			else {
					bo_log("bo_sendDataFIFO() ip[%s] wait[OK] but recv[%s]:\n%s", 
					ip,  
					buf,
					"data don't write in FIFO.");
			}
		}
		if(close(sock) == -1) {
			bo_log("bo_sendDataFIFO() when close socket errno[%s]\n ip[%s]\nport[%d]\n", 
				strerror(errno), ip, port);
		}
	} else {
		bo_log("bo_sendDataFIFO().bo_crtSock() errno[%s]\n ip[%s]\nport[%d]\nsize[%d]", 
			strerror(errno),
			ip,
			port,
			dataSize);
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	 отпр SET|LEN|DATA -> ждем ответ OK 
 * @return	[-1] - error; [1] - OK  
 */
int bo_sendSetMsg(int sock, char *data, unsigned int dataSize)
{
	int ans  = -1;
	char *head = "SET";
	ans = bo_sendXXXMsg(sock, head, data, dataSize);
	return ans;
}

/* @brief	 отпр TAB|LEN|DATA -> ждем ответ OK */
int bo_sendTabMsg(int sock, char *data, unsigned int dataSize)
{
	int ans  = -1;
	char *head = "TAB";
	ans = bo_sendXXXMsg(sock, head, data, dataSize);
	return ans;
}

/* @brief	 отпр LOG|LEN|DATA -> ждем ответ OK */
int bo_sendLogMsg(int sock, char *data, unsigned int dataSize)
{
	int ans  = -1;
	char *head = "LOG";
	ans = bo_sendXXXMsg(sock, head, data, dataSize);
	return ans;
}

/* @brief	отпр RLO|INDEX(2bytes) */
int bo_sendRloMsg(int sock, int index)
{
	int ans = -1;
	char *head = "RLO";
	unsigned char ind[2] = {0};
	int exec = -1;
	
	boIntToChar(index, ind);
	exec = bo_sendAllData(sock, (unsigned char*)head, 3);
	if(exec == -1) {
		bo_log("bo_sendRloMsg() %s send[head] errno[%s]",
			"WARN",
			strerror(errno));
		goto end;
	}
	exec = bo_sendAllData(sock, ind, 2);
	if(exec == -1) {
		bo_log("bo_sendRloMsg() %s send[index] errno[%s]",
			"WARN",
			strerror(errno));
		goto end;
	}
	ans = 1;
	end:
	return ans;
}

int bo_sendXXXMsg(int sock, char *head, char *data, int dataSize)
{
	int ans  = -1;
	int exec = -1;
	unsigned char len[2] = {0};
	/*
		char buf[4] = {0};
	*/
	unsigned char tmp[1300] = {0};
	
	boIntToChar(dataSize, len);
	/*
	exec = bo_sendAllData(sock, (unsigned char*)head, 3);
	if(exec == -1) {
		bo_log("bo_sendXXXMsg() %s send[head] errno[%s]",
			"WARN",
			strerror(errno));
		goto end;
	}
	exec = bo_sendAllData(sock, len, 2);
	if(exec == -1) {
		bo_log("bo_sendXXXMsg() %s send[len] errno[%s]",
			"WARN",
			strerror(errno));
		goto end;
	}
	exec = bo_sendAllData(sock, (unsigned char*)data, dataSize);
	if(exec == -1) {
		bo_log("bo_sendXXXMsg() %s send[data] errno[%s]",
			"WARN",
			strerror(errno));
		goto end;
	}
	*/
	memcpy(tmp, head, 3);
	memcpy(tmp + 3, len, 2);
	memcpy(tmp + 5, data, dataSize);
	exec = bo_sendAllData(sock, tmp, dataSize + 5);
	if(exec == -1) {
		bo_log("bo_sendXXXMsg() %s send[data] errno[%s]",
			"WARN",
			strerror(errno));
		goto end;
	} else ans = 1;
	
	/*
	exec = bo_recvAllData(sock, (unsigned char*)buf, 3, 3);
	if(exec == -1) {
		bo_log("bo_sendXXXMsg() %s recv ans errno[%s]",
			"WARN",
			strerror(errno));
	} else {
		if(strstr(buf, "OK")) ans = 1;
		else {
			bo_log("bo_sendXXXMsg() wait[OK] but recv[%s]:\n%s", 
				buf, "data don't send to client.");
		}
	}
	 */
	end:
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	отправка ASK серверу ждем ответ ASK от сервера
 * @return	[-1] ERROR [1] получен ответ
 */
int bo_chkSock(int sock)
{
	int ans = -1, exec = -1;
	char head[3] = "ASK";
	char buf[3] = {0};
	
	exec = bo_sendAllData(sock, (unsigned char*)head, 3);
	if(exec == -1) {
		bo_log("bo_chkSock() %s send[head] errno[%s]",
			"WARN",
			strerror(errno));
		goto end;
	}
	
	exec = bo_recvAllData(sock, (unsigned char*)buf, 3, 3);
	if(exec == -1) {
		bo_log("bo_chkSock() WARN don't recv ans");
	} else {
		if(strstr(buf, "ASK")) ans = 1;
		else bo_log("bo_chkSock() recv bad ans"); 
	}
	
end:	
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	созд сокет 
 */
int bo_crtSock(char *ip, unsigned int port, struct sockaddr_in *saddr)
{
	int sock = -1;
	struct in_addr servip;
	int i = 1;
	sock = socket(AF_INET, SOCK_STREAM, 0);
	if(sock > 0) {
		saddr->sin_family = AF_INET;
		saddr->sin_port = htons(port);
		inet_aton(ip, &servip);
		saddr->sin_addr.s_addr = servip.s_addr;
		/* Позволяет ядру повторно использовать адрес сокета.
		 * Можно запускать программу два раза подряд. Не ожидая пока 
		 * истечет ограничение на повторное испол кортежа (ip, port)(2min)
		 * SOL_SOCKET - указывает на установку опции обобщеного сокета
		 * SO_REUSEADDR - опция которая подлежит изменению
		 * &i - указатель на новое значение для опции
		 */
		i = 1;
		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	} else {
		bo_log("bo_crtSock() socket() ERROR errno[%s]", strerror(errno));
	};
	return sock;
}

/* ----------------------------------------------------------------------------
 * @brief	создание сокета -> установка таймера на прием вход данных ->
 *		установка соединения
 * @return	[-1] error [sock>0] сокет
 */
int bo_setConnect(char *ip, int port)
{
	int sock = -1;
	int n = 0;
	int conSet = 0;
	struct sockaddr_in saddr;
	int exec = -1, flag = 1;
	sock = bo_crtSock(ip, port, &saddr);

	if(sock > 0) {
		bo_setTimerRcv(sock);
		/* откл Алгоритм Нейгла */
		exec = setsockopt(sock, 
			IPPROTO_TCP, 
			TCP_NODELAY, 
			(char*)&flag,
			sizeof(flag));
		if(exec == -1) bo_log("m_addClient can't setsockopt(TCP_NODELAY)");
		while(n < 3) {
			if(connect(sock, (struct sockaddr *)&saddr, 
			   sizeof(struct sockaddr)) == 0) {
				conSet = 1; 
				break;
			} else {
				
				bo_log("bo_setConnect() n[%d] errno[%s] ip[%s] \
					port[%d] ",
					n,
					strerror(errno),
					ip,
					port);
				
				n++;
			}
			usleep(100000);
		}
		
		if(conSet != 1) {
			close(sock);
			sock = -1;
		}
		
	} else
		sock = -1;
	
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
	/* for debug*/
/*	unsigned char *ptr_deb = ptr;
	int i = 0; */
	while(allSend < len) {
		/* lock SIG_PIPE signal */
		count = send(sock, ptr + allSend, n - allSend, MSG_NOSIGNAL);
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
/*	
		unsigned char *ptr_deb = buf;
		int i = 0; 
*/	
	while(all < length) {

		count = recv(sock, buf + all, length - all, 0);
		if(count < 1) { 
			if(all != length) exec = -1;
			break;
		}
		/* info for debug 
		printf("bo_recvAllData() data:\n");
		ptr_deb = buf + all;
		for(; i < count; i++) {
			printf("%c", *(ptr_deb + i) );
		}
		 end info debug*/
		all += count;
	}
	return ( exec == -1 ? -1 : all);
}

/* ----------------------------------------------------------------------------
 * @brief		время ожид получения данных
 */
void bo_setTimerRcv(int sock)
{
	struct timeval tval;
	tval.tv_sec = 1;
	tval.tv_usec = 500000;
	/* устан максимальное время ожидания одного пакета */
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(tval));
}

void bo_setTimerRcv2(int sock, int sec, int mil)
{
	struct timeval tval;
	/* 1,5 мсек*/
	tval.tv_sec  = sec;
	tval.tv_usec = mil;
	/* устан максимальное время ожидания одного пакета */
	setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(tval));

}

/* ---------------------------------------------------------------------------
* @brief	читаем длину пакета из сокета
* @return	[-1] - ошибка, [>0] - длина сообщения
*/
unsigned int bo_readPacketLength(int sock)
{
	unsigned char buf[2] = {0};
	int count = 0;
	unsigned int ans = -1;
	/* ждем прихода 2byte опред длину сообщения */
	count = bo_recvAllData(sock, buf, 2, 2); 
	if(count == 2) {
		/*b1b2 -> b1 - старший байт
		 *	  b2 - младший байт
 		 */
		ans = boCharToInt(buf);
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @ip		сюда будет записан результат, size = 16
 * @return	[-1] ERROR [1] 
 */
int bo_getIp(int sock, char *ip) 
{
	int ans = -1, exec = -1;
	struct sockaddr_in addr;
	char *buf = NULL;
	socklen_t addr_len = sizeof(addr);
	
	exec = getpeername(sock, (struct sockaddr *)&addr, &addr_len);
	if(exec == 0) {
		if(addr_len >16) {
			bo_log("bo_getIp() ERROR addr_Len[%d] > 15", addr_len);
			goto exit;
		}
		buf = inet_ntoa(addr.sin_addr);
		memcpy(ip, buf, addr_len);
		ans = 1;
	} else {
		bo_log("ERROR bo_getIp()->getpeername() errno[%s]", 
			strerror(errno));
	}
	exit:
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief		закрытие сокета
 */
void bo_closeSocket(int sock)
{
	int exec = 0;
	if (sock > 0) {
		exec = close(sock);
		if(exec == -1) {
			bo_log("bo_closeSocket() errno[%s]", strerror(errno));
		}
	} else
		bo_log("bo_closeSocket() try close bad sock[%d]", sock);
	
}

/* 0x42 */
