#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "../tools/dbgout.h"
#include "../log/bologging.h"
#include "bo_net.h"
#include "bo_fifo.h"
#include "../tools/ocfg.h"

extern unsigned int boCharToInt(unsigned char *buf);

struct ParamSt;
static void readConfig(TOHT *cfg, int n, char **argv);

static void fifoServWork	(int sockfdMain);
static void fifoReadPacket	(int clientSock, unsigned char *buffer, 
				 int bufSize, 
				 int *endPr);
static void fifoReadHead	(struct ParamSt *param);
static void fifoQuit		(struct ParamSt *param);
static void fifoSetData		(struct ParamSt *param);
static void fifoGetData		(struct ParamSt *param);
static void fifoAnsErr		(struct ParamSt *param);
static void fifoAnsOk		(struct ParamSt *param);
static void fifoAnsNo		(struct ParamSt *param);
static void fifoAddToFIFO	(struct ParamSt *param);
static void fifoDelHead		(struct ParamSt *param);
static void fifoEnd		(struct ParamSt *param);
static void fifoMem		(struct ParamSt *param);
static unsigned int readPacketLength(struct ParamSt *param);

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
 *		SET - установить запрос в FIFO
 *		GET - забрать запрос из FIFO
 *		DEL - удалить голову в очереди
 *		END - выкл сервера
 *		ANSNO - очередь пустая
 *		QUIT  - окончание работы с клиентом
 */
static char *PacketStatusTxt[] = {"READHEAD", "SET", "GET", "QUIT", 
"ANSERR", "ANSOK", "ANSNO", "ADDFIFO", "DEL", "END", "MEM"};

static enum PacketStatus {READHEAD = 0, SET, GET, QUIT, ANSERR, ANSOK, 
	ANSNO, ADDFIFO, DEL, END, MEM} packetStatus;
	
/* массив содерж указатели на функции(возвращают void;
 * аргумент у функций указ на struct ParamSt) */	
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
	fifoEnd,
	fifoMem
};

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
 *			request: SET|GET|DEL|MEM
 *			response: OK|ERR|
 */
void bo_fifo_main(int n, char **argv)
{
	int sock = 0;
	TOHT *cfg = NULL;
	readConfig(cfg, n, argv);
	bo_log("%s%s", " INFO ", "START moxa_serv");

	if( (sock = bo_servStart(fifoconf.port, fifoconf.queue_len)) != -1) {
		if(bo_initFIFO(fifoconf.fifo_len) == 1) {
			fifoServWork(sock); 
			bo_delFIFO();
		}
		bo_closeSocket(sock);
	}
	if(cfg != NULL) cfg_free(cfg);
	
	bo_log("%s%s", " INFO ", "END	moxa_serv");
};
/* ----------------------------------------------------------------------------
 * @brief		Читаем данные с конфиг файла
 */
static void readConfig(TOHT *cfg, int n, char **argv)
{
	int defP = 8888, defQ = 20, defF = 100;
	char *fileName	= NULL;
	char *f_log	= "moxa_serv.log";
	char *f_log_old = "moxa_serv.log(old)";
	int  nrow   = 0;
	int  maxrow = 1000;
	fifoconf.port      = defP;
	fifoconf.queue_len = defQ;
	fifoconf.fifo_len  = defF;
	
	if(n == 2) {
		fileName = *(argv + 1);
		cfg = cfg_load(fileName);
		if(cfg != NULL) {
			fifoconf.port      = cfg_getint(cfg, "sock:port", defP);
			fifoconf.queue_len = cfg_getint(cfg, "sock:queue_len", defQ);
			fifoconf.fifo_len  = cfg_getint(cfg, "fifo:len", defF);
			
			f_log	  = cfg_getstring(cfg, "log:file", f_log);
			f_log_old = cfg_getstring(cfg, "log:file_old", f_log_old);
			maxrow    = cfg_getint(cfg, "log:maxrow", maxrow);
		} else {
			bo_log(" WARNING error[%s] %s", 
				"can't read config file",
				"start with default config");
		}
		bo_setLogParam(f_log, f_log_old, nrow, maxrow);
		bo_log("%s config[%s]", " INFO ", fileName);
	} else {
		bo_log("%s", " WARNING start with default config");
	}
 
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
  * @clientSock		дескриптор сокета(клиента) При завершение сокет закр-ся.
  * @buffer		буфер в который пишем данные. 
  * @bufSize		bufSize = BO_FIFO_ITEM_VAL(bo_fifo.h)
  * @endPr		флаг прекращ работы сервера и заверш программы
  */
static void fifoReadPacket(int clientSock, unsigned char *buffer, int bufSize,
			int *endPr)
{
	int stop = -1;
	int i = 1;
	struct ParamSt param;
	param.clientfd = clientSock;
	param.buffer = buffer;
	param.bufSize = bufSize;
	packetStatus = READHEAD;
	/* устан максимальное время ожидания одного пакета */
	bo_setTimerRcv(clientSock);
	/* при перезапуске программы позволяет повторно использовать адрес 
	 * порт, иначе придется ждать 2MSL */
	setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
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
	bo_closeSocket(clientSock);
	dbgout("\n> ----------- END CONNECT ------------ <\n");
}
 /* ---------------------------------------------------------------------------
  * @brief		чтение заголовка запроса
  *			SET|GET|DEL|END
  * @client		сокет соот-ий текущ соед-ию
  * @return		[1] = ok, [-1] = error
  */
 static void fifoReadHead(struct ParamSt *param)
 {
	int bufSize = 3;
	char buf[bufSize + 1];
	int exec = 0;
	buf[bufSize] = '\0';
	exec = bo_recvAllData(param->clientfd, (unsigned char *)buf, bufSize, 3);
	if(exec == -1) {
		bo_log("fifoReadHead() errno[%s]", strerror(errno));
	}
	if(exec == 3) {
		dbgout("fifoReadHead()->HEAD[%s] exec=%d", buf, exec);
		if(strstr(buf, "SET")) packetStatus = SET;
		else if(strstr(buf, "GET")) packetStatus = GET;
		else if(strstr(buf, "DEL")) packetStatus = DEL;
		else if(strstr(buf, "MEM")) packetStatus = MEM;
		else if(strstr(buf, "END")) packetStatus = END;
		else packetStatus = ANSERR;
	} else {
		if(exec > 0 ) dbgout("fifoReadHead()->HEAD[%s] exec=%d", buf, exec);
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
	int count = 0;
	length = readPacketLength(param);
	if((length > 0) & (length <= param->bufSize)) {
		count = bo_recvAllData(param->clientfd, 
				       param->buffer,
			               param->bufSize,
				       length);
		if((count > 0) & (count == length)) flag = 1; 
		else {
			bo_log("fifoSetData() count[%d]!=length[%d]", count, length);
		}
	} else {
		bo_log("fifoSetData() bad length[%d] bufSize[%d]", 
			length, 
			param->bufSize);
	}
	if(flag == 1) {
		param->packetLen = length;
		packetStatus = ADDFIFO;
	} else {
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
	int i = 0;
	unsigned char len[2] = {0};
	unsigned char head[3] = "VAL";
	unsigned char headNO[3] = " NO"; 
	bo_printFIFO();

	param->packetLen = bo_getFIFO(param->buffer, param->bufSize);
	boIntToChar(param->packetLen, len);
	
	dbgout("param->packetLen=%d\n", param->packetLen);
	printf("get from FIFO buf:");
	for(i = 0; i < param->packetLen; i++) {
		printf("%c ", *(param->buffer + i) );
	}
	printf("\n");
	if(param->packetLen > 0) {
		exec = bo_sendAllData(param->clientfd, head, 3);
		if(exec == -1){
			bo_log("fifoGetData()send[VAL] errno[%s]", strerror(errno));
			goto exit;
		}
		
		exec = bo_sendAllData(param->clientfd, len, 2);
		if(exec == -1) { 
			bo_log("fifoGetData()send[len] errno[%s]", strerror(errno));
			goto exit;
		}
		exec = bo_sendAllData(param->clientfd, param->buffer, 
			param->packetLen);
		if(exec == -1) {
			bo_log("fifoGetData()send[data] errno[%s]", strerror(errno));
			goto exit;
		}
	} else {
		exec = bo_sendAllData(param->clientfd, headNO, 3);
		bo_log("fifoGetData()send[NO] errno[%s]", strerror(errno));
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
	int exec = -1; 
	int sock = param->clientfd;
	unsigned char msg[] = " OK";
	packetStatus = QUIT;

	exec = bo_sendAllData(sock, msg, 3);
	if(exec == -1) bo_log("fifoAnsOk() errno[%s]", strerror(errno)); 
}
 /* ---------------------------------------------------------------------------
  * @brief		отправляем ответ " NO" (нет данных в очереди)
  */
static void fifoAnsNo(struct ParamSt *param)
{
	int exec = -1; 
	int sock = param->clientfd;
	unsigned char msg[] = " NO";
	packetStatus = QUIT;
		
	exec = bo_sendAllData(sock, msg, 3);
	if(exec == -1) bo_log("fifoAnsNo() errno[%s]", strerror(errno));
}
/* --------------------------------------------------------------------------
 * @brief		send answer "ERR"
 */
static void fifoAnsErr(struct ParamSt *param)
{
	int exec = -1;
	int sock = param->clientfd;
	unsigned char msg[] = "ERR";
	packetStatus = QUIT;
	
	exec = bo_sendAllData(sock, msg, sizeof(msg));
	if(exec == -1) bo_log("fifoAnsErr() errno[%s]", strerror(errno));
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
		printf("%c ", param->buffer[i]);
	}
	printf("\n");
		
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
/* ----------------------------------------------------------------------------
 * @brief		пишем в файд memory.trace состояние памяти
 *			(mstats - not exists see mallinfo <- 0x42) 
 */
static void fifoMem(struct ParamSt *param)
{
	packetStatus = QUIT;
}
 /* ---------------------------------------------------------------------------
  * @brief	читаем длину пакета
  * @return	[-1] - ошибка, [>0] - длина сообщения
  */
 static unsigned int readPacketLength(struct ParamSt *param)
 {
	int sock = param->clientfd;
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

 
 
 
 