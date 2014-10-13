#include <stdlib.h>
#include <string.h>
#include "../tools/dbgout.h"
#include "../log/bologging.h"
#include "bo_net.h"
#include "bo_fifo.h"

extern unsigned int boCharToInt(unsigned char *buf);

struct ParamSt;
static int readConfig();
static int fifoServStart();
static void fifoServWork(int sockfdMain);
static void fifoReadPacket(int clientSock, unsigned char *buffer, int bufSize, 
			   int *endPr);
static void fifoReadHead(struct ParamSt *param);
static void fifoQuit(struct ParamSt *param);
static void fifoSetData(struct ParamSt *param);
static void fifoGetData(struct ParamSt *param);
static void fifoAnsErr(struct ParamSt *param);
static void fifoAnsOk(struct ParamSt *param);
static void fifoAnsNo(struct ParamSt *param);
static void fifoAddToFIFO(struct ParamSt *param);
static void fifoDelHead(struct ParamSt *param);
static void fifoEnd(struct ParamSt *param);
static unsigned int readPacketLength(struct ParamSt *param);
static int readPacketBody(struct ParamSt *param, unsigned int length);
/* ----------------------------------------------------------------------------
 * @port	- порт накотором висит слуш сокет
 * @queue_len	- кол-во необр запросов в очереди, при перепол вохзвращает 
 *		  клиенту ECONREFUSED <-(0x42:уточнить?)
 * @fifo_len	- размер FIFO для хран запросов 485  
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
static char *PacketStatusTxt[] = {"READHEAD", "SET", "GET", "QUIT", 
"ANSERR", "ANSOK", "ANSNO", "ADDFIFO", "DEL"};
static enum PacketStatus {READHEAD = 0, SET, GET, QUIT, ANSERR, ANSOK, 
	ANSNO, ADDFIFO, DEL, END} packetStatus;
static void(*statusTable[])(struct ParamSt *) = {
	fifoReadHead, 
	fifoSetData, 
	fifoGetData,
	fifoQuit,
	fifoAnsErr,
	fifoAnsOk,
	fifoAnsNo,
	fifoAddToFIFO,
	fifoDelHead,
	fifoEnd};
/* @packetLen		длина запроса для FIFO
 * @clientfd		сокет 
 * @buffer		буфер с запросом для FIFO
 * @bufSize		размер буфера
 */
struct ParamSt {
	int packetLen;
	int clientfd;
	unsigned char *buffer;
	int bufSize;
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
			if(bo_initFIFO(fifoconf.fifo_len) == 1) {
				fifoServWork(sock); 
				bo_delFIFO();
			}
			if(close(sock) == -1) {
				bo_log("%s%s errno[%s]", 
					" ERROR ",
					"bo_fifo_main()->close()",
					strerror(errno));
			}
		}
	} else {
		bo_log("%s%s", " ERROR ", " readConfig() can't run server fifo");
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
	fifoconf.fifo_len = 100;
	return 1;
 };
/* ----------------------------------------------------------------------------
 * @brief		запуск слушающего сокета
 * @return		[sockfd] - сокет; [-1] - error
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
	/* размер ячейки в FIFO */
	int bufferSize = BO_FIFO_ITEM_VAL;
	unsigned char *buffer1 = NULL;
	bo_log("%s%s", " INFO ", " RUN ");
	/* создаем буфер */
	buffer1 = (unsigned char *)malloc(bufferSize);
	if(buffer1 == NULL) {
		bo_log("%s%s", " ERROR ", "fifoServWork() can't create buffer1");
		return;
	}
	while(stop == 1) {
		if(bo_waitConnect(sockfdMain, &clientfd, &errTxt) == 1) {
			countErr = 0;
			/* передаем флаг stop. Ф-ия может поменять его на -1
			   если придет пакет END */
			fifoReadPacket(clientfd, buffer1, bufferSize, &stop);
		} else {
			countErr++;
			bo_log("%s%s errno[%s]", " ERROR ", 
				"fifoServWork()", errTxt);
			if(countErr == 10) stop = -1;
		}
	}
	free(buffer1);
 }
 /* ---------------------------------------------------------------------------
  * @brief		читаем пакет и пишем/забираем/удаляем в/из FIFO
  * @clientSock		дескриптор сокета(клиента)
  * @buffer		буфер в который пишем данные. 
  * @bufSize		bufSize = BO_FIFO_ITEM_VAL(bo_fifo.h)
  * @endPr		флаг прекращ работы сервера и заверш программы
  */
static void fifoReadPacket(int clientSock, unsigned char *buffer, int bufSize,
			int *endPr)
{
	int stop = -1;
	struct ParamSt param;
	struct timeval tval;
	param.clientfd = clientSock;
	param.buffer = buffer;
	param.bufSize = bufSize;
	packetStatus = READHEAD;
	/* 100 мсек*/
	tval.tv_sec = 0;
	tval.tv_usec = 100000;
	/* устан максимальное время ожидания одного пакета */
	setsockopt(clientSock, SOL_SOCKET, SO_RCVTIMEO, &tval, sizeof(tval));
	dbgout("\n> ----------	CONNECT ---------------- <\n");
	while(stop == -1) {
		dbgout("\n>>>AVTOMAT STATUS = %s\n", PacketStatusTxt[packetStatus]);
		if(packetStatus == QUIT) break;
		if(packetStatus == END) {
			*endPr = -1;
			break;
		}
		statusTable[packetStatus](&param);
	}
	close(param.clientfd);
	dbgout("\n> ----------- END CONNECT ------------ <\n");
}
 /* ---------------------------------------------------------------------------
  * @brief		чтение заголовка запроса
  *			SET|GET|DEL|ASK|ERR
  * @client		сокет соот-ий текущ соед-ию
  * @return		[1] = ok, [-1] = error
  */
 static void fifoReadHead(struct ParamSt *param)
 {
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
		else if(strstr(buf, "GET")) packetStatus = GET;
		else if(strstr(buf, "DEL")) packetStatus = DEL;
		else if(strstr(buf, "END")) packetStatus = END;
		else packetStatus = ANSERR;
	} else {
		packetStatus = ANSERR;
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
	int flag = -1;
	length = readPacketLength(param);
	if((length > 0) & (length <= param->bufSize)) {
		flag = readPacketBody(param, length);
	}
	if(flag == 1) {
		param->packetLen = length;
		packetStatus = ADDFIFO;
	} else {
		bo_log("fifoSetData() body == NULL or length[%d] > bufSize[%d]\n", 
			length, param->bufSize);
		packetStatus = ANSERR;
	}
 }
 /* ---------------------------------------------------------------------------
  * @brief		снимаем со стека 1 запрос и отправляем клиенту
  * status -> READHEAD |   вслучае успеной отправки ждем ответ от клиента
  *             QUIT   |   если возникла ошибка при передаче
  *			   если стек пустой отправляем " NO" и выходим
  */
 static void fifoGetData(struct ParamSt *param)
 {
	int exec = -1;
	unsigned char len[2] = {0};
	unsigned char head[3] = "VAL";
	unsigned char headNO[3] = " NO"; 
	boIntToChar(param->packetLen, len);
	dbgout("fifo free item=%d\n", bo_getFree());
	param->packetLen = bo_getFIFO(param->buffer, param->bufSize);
	if(param->packetLen > 0) {
		exec = bo_sendAllData(param->clientfd, head, 3);
		if(exec == -1) goto exit;
		exec = bo_sendAllData(param->clientfd, len, 2);
		if(exec == -1) goto exit;
		exec = bo_sendAllData(param->clientfd, param->buffer, 
			param->packetLen);
	} else {
		exec = bo_sendAllData(param->clientfd, headNO, 3);
		goto exit;
	}
	if(exec == -1) {
		exit:
		packetStatus = QUIT;
	} else {
		packetStatus = READHEAD;
	}
	return;
 }
 /* ---------------------------------------------------------------------------
  * @brief		empty functions
  */
 static void fifoQuit(struct ParamSt *param) { }
 static void fifoEnd(struct ParamSt *param) { }
 /* ---------------------------------------------------------------------------
  * @brief		send answer " OK"
  */
 static void fifoAnsOk(struct ParamSt *param)
 {
	int sock = param->clientfd;
	unsigned char msg[] = " OK";
	bo_sendAllData(sock, msg, 3);
	packetStatus = QUIT;
 }
 /* ---------------------------------------------------------------------------
  * @brief		отправляем ответ " NO" (нет данных в очереди)
  */
 static void fifoAnsNo(struct ParamSt *param)
 {
	int sock = param->clientfd;
	unsigned char msg[] = " NO";
	bo_sendAllData(sock, msg, 3);
	packetStatus = QUIT;
 }
  /* --------------------------------------------------------------------------
  * @brief		send answer "ERR"
  */
 static void fifoAnsErr(struct ParamSt *param)
 {
	int sock = param->clientfd;
	unsigned char msg[] = "ERR";
	bo_sendAllData(sock, msg, sizeof(msg));
	packetStatus = QUIT;
 }
 /* ---------------------------------------------------------------------------
  * @brief	добавляем данные в FIFO
  */
 static void fifoAddToFIFO(struct ParamSt *param)
 {
	int flag = -1;
	int i = 0;
	dbgout("--- --- fifoAddToFIFO len%d val:\n", param->packetLen);
	for(; i < param->packetLen; i++) {
		printf(" 0x%02x", param->buffer[i]);
	}
	printf("\n");
	
	dbgout("fifo free item = %d\n", bo_getFree());
	
	flag = bo_addFIFO(param->buffer, param->packetLen);
	if(flag == -1) {
		packetStatus = ANSERR;
	} else {
		packetStatus = ANSOK;
	}
 }
 /* ---------------------------------------------------------------------------
  * @brief		отправляем сообщение OK если сообщ отправ то удал Head
  * status -> QUIT			
  */
 static void fifoDelHead(struct ParamSt *param)
 {
	int exec = -1;
	unsigned char msg[3] = " OK";
	exec = bo_sendAllData(param->clientfd, msg, sizeof(msg));
	if(exec != -1) bo_delHead();
	packetStatus = QUIT;
 }
 /* ---------------------------------------------------------------------------
  * @brief	читаем длину пакета
  * @return	[0] - ошибка, [>0] - длина сообщения
  */
 static unsigned int readPacketLength(struct ParamSt *param)
 {
	int sock = param->clientfd;
	char buf[2] = {0};
	int count = 0;
	unsigned int ans = 0;
	/* ждем прихода 2byte опред длину сообщения*/
	count = recv(sock, buf, 2, 0); 
	if(count == 2) {
		/*b1b2 -> b1 - старший байт
		 *	  b2 - младший байт
 		 */
		ans = boCharToInt((unsigned char *)buf);
	}
	return ans;
 }
 /* ---------------------------------------------------------------------------
  * @brief	читаем тело пакета
  * @length	длина пакета
  * @return	[-1] - ERROR [1] - OK
  */
 static int readPacketBody(struct ParamSt *param,
	 unsigned int length)
 {
	int ans = 1;
	unsigned char *ptr_poz = NULL;
	int sock = param->clientfd;
	size_t count = 0;
	int sizeBuf = 256;
	int N = 0;
	char buf[256] = {0};
	ptr_poz = param->buffer;
	while(1) {
		if(N == length) break;
		count = recv(sock, buf, sizeBuf, 0);
		if((N + count) > length) {
			ans = -1;
			goto exit;
		}
		if(count < 0) {
			if((errno == EWOULDBLOCK) | (errno == EAGAIN)) 
				dbgout("timeout\n"); 
			dbgout("readPacketBody() errno[%s]\n", strerror(errno));
			ans = -1;
			goto exit;
		}
		if(count == length) {
			memcpy(ptr_poz, buf, count);
			N += count;
			break;
		} else if(count > length) {
			ans = -1;
			goto exit;
		} else if( (count < length) & (count > 0)) {
			memcpy(ptr_poz, buf, count);
		}
		N += count;
		ptr_poz = param->buffer + N;
	}
	if(N != length) {
		ans = -1;
	}
	exit:
	return ans;
 }

 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 
 