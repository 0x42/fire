#include <stdlib.h>
#include <string.h>
#include "../tools/dbgout.h"
#include "../log/bologging.h"
#include "bo_net.h"

extern unsigned int boCharToInt(unsigned char *buf);

struct ParamSt;
static int readConfig();
static int fifoServStart();
static void fifoServWork(int sockfdMain);
static void fifoReadPacket(int sockfd);
static void fifoReadHead(struct ParamSt *param);
static void fifoError(struct ParamSt *param);
static void fifoQuit(struct ParamSt *param);
static void fifoSetData(struct ParamSt *param);
static void fifoGetData(struct ParamSt *param);
static unsigned int readPacketLength(struct ParamSt *param);
static unsigned char *readPacketBody(struct ParamSt *param, unsigned int length);
/* ----------------------------------------------------------------------------
 * @port	- порт накотором висит слуш сокет
 * @queue_len	- кол-во необр запросов в очереди, при перепол вохзвращает 
 *		  клиенту ECONREFUSED <-(0x42:уточнить?)
 * @fifo_len	- размер FIFO запросо для 485
 * @clientfd	- дескриптор сокета от устан соед с клиентом
 */
static struct {
	int port;	
	int queue_len;
	int fifo_len; 
	int clientfd;
} fifoconf = {0};
/* ----------------------------------------------------------------------------
 * @brief		состояние сервера
 *		ERR - ошибка
 *		WAIT - ожидание нового подкл 
 *		SETFIFO - установить запрос в FIFO
 *		GETFIFO - забрать запрос из FIFO
 */
static char *PacketStatusTxt[] = {"FIFOERR", "READHEAD", "SET", "GET", "QUIT"};
static enum PacketStatus {FIFOERR=0, READHEAD, SET, GET, QUIT} packetStatus;
static void(*statusTable[])(struct ParamSt *) = {
	fifoError, 
	fifoReadHead, 
	fifoSetData, 
	fifoGetData,
	fifoQuit};
struct ParamSt {
	int packetLen;
	int clientfd;
};
/* ----------------------------------------------------------------------------
 * @brief		Запуск сервера FIFO
 *			request: SET|GET|DEL|ASK
 *			response: OK|ERR|
 */
void bo_fifo_main()
{
	int sock = 0;
	bo_log("%s%s", " INFO ", "START serverfifo");
	/*read config file*/
	if(readConfig() == 1) {
		if( (sock = fifoServStart()) != -1) {
			fifoServWork(sock);
			if(close(sock) == -1) {
				bo_log("%s%s errno[%s]", 
					" ERROR ",
					"bo_fifo_main()->close()",
					strerror(errno));
			}
		}
	} else {
		bo_log("%s%s", " ERROR ", " readConfid() can't run server fifo");
	}
	bo_log("%s%s", " INFO ", "END	serverfifo");
};
/* ----------------------------------------------------------------------------
 * @brief		Читаем данные с конфиг файла
 * @return		[1] - ok; [-1] - error
 */
 static int readConfig()
 {
	fifoconf.port = 8888;
	fifoconf.queue_len = 20;
	fifoconf.fifo_len = 1000;
	return 1;
 };
/* ----------------------------------------------------------------------------
 * @brief		запуск слушающего сокета
 * @sock		присваивается дескр сокета вслучае успеха  
 * @return		[sockfd] - ok; [-1] - error
 */ 
 static int fifoServStart()
 {
	int ans = -1;
	int sockfd = 0;
	char *errTxt = NULL;
	sockfd = bo_initServSock(fifoconf.port, &errTxt);
	if(sockfd > 0) {
		if(bo_setListenSock(sockfd, fifoconf.queue_len, &errTxt) != 1) {
			bo_log("%s%s errno[%s]", " ERROR ", 
				"fifoServStart() ", errTxt);
			close(sockfd);
			ans = -1;
		} else {
			ans = sockfd;
		}
	} else {
		bo_log("%s%s errno[%s]", " ERROR ", 
			"fifoServStart()->bo_initServSock()", errTxt);
		ans = -1;
	}
	return ans;
 };
 /* ---------------------------------------------------------------------------
  * @brief		осн цикл сервера. вход соед и отодает его worker'у
  * @sock		дескриптор главного сокета на который поступают
  *			соединения. Не закрывать его!!!
  */
static void fifoServWork(int sockfdMain)
 {
	int stop = 1;
	int countErr  = 0;
	char *errTxt = NULL;
	int clientfd = 0;
	bo_log("%s%s %d", " INFO ", " START LOOP", sockfdMain);
	while(stop == 1) {
		if(bo_waitConnect(sockfdMain, &clientfd, &errTxt) == 1) {
			countErr = 0;
			fifoReadPacket(clientfd);
		} else {
			countErr++;
			bo_log("%s%s errno[%s]", " ERROR ", 
				"fifoServWork()", errTxt);
			if(countErr == 10) stop = -1;
		}
	} 
 }
 /* ---------------------------------------------------------------------------
  * @brief		читаем пакет
 */
static void fifoReadPacket(int clientSock)
{
	int stop = -1;
	struct ParamSt param;
	struct timeval tval;
	param.clientfd = clientSock;
	packetStatus = READHEAD;
	tval.tv_sec = 0;
	tval.tv_usec = 100000;
	setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(tval));
	while(stop == -1) {
		dbgout("status = %s\n", PacketStatusTxt[packetStatus]);
		if(packetStatus == QUIT) break;
		statusTable[packetStatus](&param);
	}
	close(param.clientfd);
}
 /* ---------------------------------------------------------------------------
  * @brief		чтение заголовка запроса
  *			SET|GET|DEL|ASK|ERR
  * @client		сокет соот-ий текущ соед-ию
  * @return		[1] = ok, [-1] = error
  */
 static void fifoReadHead(struct ParamSt *param)
 {
	printf("%s -> fifoReadHead()\n", PacketStatusTxt[packetStatus]);
	int headSize = 3;
	char buf[headSize + 1];
	int  count = 0;
	int exec = 0;
	/*  в течение 1 сек ждем прихода headSize байт */
	while(exec != 100) {
		/* MSG_PEEK после чтения данных с сокета их копия 
		 * остается в очереди */
		count = recv(param->clientfd, buf, headSize, MSG_PEEK);
		if(count == 3)  break;
		usleep(100000);
		exec++;
	}
	memset(buf, 0, headSize);
	count = recv(param->clientfd, buf, headSize, 0);
	buf[headSize] = '\0';
	dbgout("fifoReadHead()->HEAD[%s] count=%d", buf, count);
	if(count == -1) dbgout("errno[%s]", strerror(errno));
	if(count == 3) {
		if(strstr(buf, "SET")) packetStatus = SET;
		else if(strstr(buf, "GET")) packetStatus= GET;
	} else {
		packetStatus = QUIT;
	}
 }
 /* ---------------------------------------------------------------------------
  * @brief		читаем длину запроса, его тело и кладем в FIFO
  * @param		ParamSt {packetLen = кол-во символов опред длину пакета
  *				 clientfd = сокет клиента}
  */
 static void fifoSetData(struct ParamSt *param)
 { 
	unsigned int length = 0;
	int i = 0;
	unsigned char *body;
	printf("fifoSetData()\n");
	length = readPacketLength(param);
	printf("length = %d\n", length);
	printf(" <-- start read body\n");
	body = readPacketBody(param, length);
	printf(" <-- end read body\n");
	if(body != NULL) {
		printf(" ... get body ... \n");
//		for(i = 0; i < length; i++) {
//			printf(" i:%d [%02x]\n", i, *(body + i));
//		}
		free(body);
	} else {
		printf("body == NULL \n");
	}
	packetStatus = QUIT;
 }
 /* ---------------------------------------------------------------------------
  * @brief		снимаем со стека 1 запрос и отправляем клиенту
  */
 static void fifoGetData(struct ParamSt *param)
 {
	 printf("fifoGetData()\n" );
	 packetStatus = QUIT;
 }
 /* ---------------------------------------------------------------------------
  * @brief		
  */
 static void fifoError(struct ParamSt *param)
 {
	 printf("fifoERROR() \n" );
	 packetStatus = QUIT;
 }
 /* ---------------------------------------------------------------------------
  * @brief		empty function
  */
 static void fifoQuit(struct ParamSt *param) { }
 /* ---------------------------------------------------------------------------
  * @brief	читаем длину пакета
  * @return	[0] - ошибка, [>0] - длина сообщения
  */
 static unsigned int readPacketLength(struct ParamSt *param)
 {
	 int sock = param->clientfd;
	 char buf[2] = {0};
	 int count = 0;
	 int exec = 0;
	 unsigned int ans = 0;
	 /* в течение 1 сек ждем прихода 2byte опред длину сообщения*/
	 while(exec != 10) {
		 count = recv(sock, buf, 2, MSG_PEEK); 
		 if(count == 2) break;
		 usleep(100000);
		 exec++;
	 }
	 count = recv(sock, buf, 2, 0);
	 if(count == 2) {
		 /*b1b2 -> b1 - старший байт
		  *	   b2 - младший байт
 		  */
		 ans = boCharToInt((unsigned char *)buf);
	 }
	 return ans;
 }
 /* ---------------------------------------------------------------------------
  * @brief	читаем тело пакета
  * @length	длина пакета
  */
 static unsigned char *readPacketBody(struct ParamSt *param, unsigned int length)
 {
	 unsigned char *msg = NULL;
	 int sock = param->clientfd;
	 size_t count = 0;
	 int sizeBuf = 10;
	 int N = 0;
	 char buf[10] = {0};
	 unsigned char *msgBuf = malloc(length * sizeof(unsigned char));
	 if(msgBuf == NULL) {
		 dbgout("readPacketBody() malloc error length[%d]", length);
		 goto exit;
	 }
	 msg = msgBuf;
	 while(1) {
		 if(N == length) break;
		 count = recv(sock, buf, sizeBuf, 0);

		 if(count < 0) {
			 if((errno == EWOULDBLOCK) | (errno == EAGAIN)) printf("timeout\n"); 
			 dbgout("readPacketBody() errno[%s]\n", strerror(errno));
			 free(msg);
			 msg = NULL;
			 goto exit;
		 }
		 if(count == length) {
			 memcpy((msg + N), buf, count);
			 N += count;
			 break;
		 } else if(count > length) {
			 free(msg);
			 msg = NULL;
			 goto exit;
		 } else if( (count < length) & (count > 0)) {
			 memcpy((msg + N), buf, count);
		 }
		 N += count;
		printf("N = %d, count = %d\n", N, (int)count);
	 }
	 if(N != length) {
		 free(msg);
		 msg = NULL;
	 }
	 exit:
	 return msg;
 }
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 