#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "../tools/dbgout.h"
#include "../log/bologging.h"
#include "bo_net.h"
#include "bo_fifo.h"
#include "../tools/ocfg.h"
#include "../tools/oht.h"

extern unsigned int boCharToInt	(unsigned char *buf);

struct ParamSt;
static void readConfig		(TOHT *cfg, int n, char **argv);

static void fifoServWork	();
static void fifoReadPacket	(int clientSock, unsigned char *buffer, 
				 int bufSize, int *endPr, TOHT *tab);
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

static unsigned int readPacketLength	(struct ParamSt *param);
static int bo_checkDblMsg		(struct ParamSt *param);
static int bo_chkDbl_setMark		(struct ParamSt *param);

/* ----------------------------------------------------------------------------
 * @port	- порт на котором висит слуш сокет
 * @queue_len	- кол-во необр запросов в очереди, при перепол возвращает 
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
 * @id_msg		Хеш табл IP-ID
 * @id			текущ ID пол-го сообщения
 */
struct ParamSt {
	char ip[16];
	int packetLen;
	int clientfd;
	unsigned char *buffer;
	int bufSize;
	TOHT *id_msg;
	char *id;
};
/* ----------------------------------------------------------------------------
 * @brief	запуск сервера FIFO в потоке
 * @port	порт сервера
 * @fifo_len    размер очереди FIFO
 * @queue_len   размер очереди на сокете
 */
void bo_fifo_thrmode(int port, int queue_len, int fifo_len)
{
	bo_log(" %s %s %s", "FIFO", "INFO", "START(THR mode) moxa_serv");
	fifoconf.port      = port;
	fifoconf.queue_len = queue_len;
	fifoconf.fifo_len  = fifo_len;
	
	if(bo_initFIFO(fifoconf.fifo_len) == 1) {
		fifoServWork(); 
		bo_delFIFO();
	}
	
	bo_log("%s %s %s", "FIFO", "INFO", "END	moxa_serv");
}

/* ----------------------------------------------------------------------------
 * @brief		Запуск сервера FIFO
 *			request: SET|GET|DEL|MEM|END
 *			response: OK|ERR
 *			PACKET = [SET|LEN|ID|VALUE|CRC]
 */
void bo_fifo_main(int n, char **argv)
{
	TOHT *cfg = NULL;
	readConfig(cfg, n, argv);
	bo_log(" %s %s %s", "FIFO", "INFO", "START moxa_serv");

	if(bo_initFIFO(fifoconf.fifo_len) == 1) {
		fifoServWork(); 
		bo_delFIFO();
	}

	if(cfg != NULL) cfg_free(cfg);
	
	bo_log("%s %s %s", "FIFO", "INFO", "END	moxa_serv");
};

/* ----------------------------------------------------------------------------
 * @brief		Читаем данные с конфиг файла
 */
static void readConfig(TOHT *cfg, int n, char **argv)
{
	int defP = 8888, defQ = 20, defF = 100;
	char *fileName	= NULL;
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
		} else {
			bo_log(" %s WARNING error[%s] %s",
				"FIFO",
				"can't read config file",
				"start with default config");
		}
		bo_log(" %s %s config[%s]", "FIFO", "INFO", fileName);
	} else {
		bo_log(" %s %s", "FIFO", " WARNING start with default config");
	} 
};

 /* ---------------------------------------------------------------------------
  * @brief		осн цикл сервера. вход соед и отдает его worker'у
  */
static void fifoServWork()
{
	int sockfdMain = -1;
	int stop = 1;
	int countErr  = 0;
	char *errTxt = NULL;
	int clientfd = 0;
	/* размер ячейки в FIFO */
	int bufferSize = BO_FIFO_ITEM_VAL;
	unsigned char *buffer1 = NULL;
	TOHT *tab = NULL;
	bo_log(" %s %s %s", "FIFO", "INFO", " RUN ");
	
	/* создаем буфер */
	buffer1 = (unsigned char *)malloc(bufferSize);
	if(buffer1 == NULL) {
		bo_log(" %s %s %s", "FIFO","ERROR", "fifoServWork() can't create buffer1");
		goto exit;
	}
	/* таблица IP-IDmsg*/
	tab = ht_new(10);
	if(tab == NULL) {
		bo_log(" %s %s %s", "FIFO","ERROR", "fifoServWork() can't create tab");
		goto exit;
	}
	
	sockfdMain = -1;
	while(stop == 1) {
		
		if( sockfdMain  != -1 ) {
			if(bo_waitConnect(sockfdMain, &clientfd, &errTxt) == 1) {
				countErr = 0;
				/* передаем флаг stop. Ф-ия может поменять его на -1
				   если придет пакет END */
				fifoReadPacket(clientfd, buffer1, bufferSize, &stop, tab);
			} else {
				countErr++;
				bo_log(" %s %s %s errno[%s]", "FIFO", "ERROR", 
					"fifoServWork()", errTxt);

			}
		} else {
			sleep(5);
			sockfdMain = bo_servStart(fifoconf.port, fifoconf.queue_len);
		}
		if(countErr == 5) {
			bo_log(" FIFO ERROR fifoServWork() restart server socket");
			countErr = 0;
			bo_closeSocket(sockfdMain);
			sockfdMain = -1;
		}
	}
	
	bo_closeSocket(sockfdMain);
	exit:
	if(tab != NULL) ht_free(tab);
	if(buffer1 != NULL) free(buffer1);
}
 
/* ---------------------------------------------------------------------------
  * @brief		читаем пакет и пишем/забираем/удаляем в/из FIFO
  * @clientSock		дескриптор сокета(клиента) При завершении сокет закр-ся.
  * @buffer		буфер в который пишем данные. 
  * @bufSize		bufSize = BO_FIFO_ITEM_VAL(bo_fifo.h)
  * @endPr		флаг прекращ работы сервера и заверш программы
 *  @tab		хранит значен IP - ID сообщения(для искл дублирования)
  */
static void fifoReadPacket(int clientSock, unsigned char *buffer, int bufSize,
			int *endPr, TOHT *tab)
{
	int stop = -1;
	int i = 1;
	struct ParamSt param;
	char idBuf[9] = {0};
	int exec = -1;
	
	param.clientfd = clientSock;
	memset(param.ip, 0, 16);
	param.buffer   = buffer;
	param.bufSize  = bufSize;
	param.id_msg   = tab;
	param.id       = idBuf;
	packetStatus   = READHEAD;
	
	/* устан максимальное время ожидания одного пакета */
	bo_setTimerRcv(clientSock);
	
	/* при перезапуске программы позволяет повторно использовать адрес 
	 * порт, иначе придется ждать 2MSL */
	setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	
	exec = bo_getIp(clientSock, param.ip);
	if(exec == -1) memset(param.ip, '-', 15);
	while(stop == -1) {
		/*dbgout("\nFIFO = %s\n", PacketStatusTxt[packetStatus]);
		*/
		if(packetStatus == QUIT) break;
		if(packetStatus == END) {
			*endPr = -1;
			break;
		}
		statusTable[packetStatus](&param);
	}
	
	bo_closeSocket(clientSock);	
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
		bo_log("%s fifoReadHead() errno[%s]", "FIFO", strerror(errno));
	}
	if(exec == 3) {
		if(strstr(buf, "SET")) packetStatus = SET;
		else if(strstr(buf, "GET")) packetStatus = GET;
		else if(strstr(buf, "DEL")) packetStatus = DEL;
		else if(strstr(buf, "MEM")) packetStatus = MEM;
		else if(strstr(buf, "END")) packetStatus = END;
		else packetStatus = ANSERR;
	} else {
		if(exec > 0 ) {
			bo_log("%s fifoReadHead() recv bad HEAD[%s]", "FIFO", buf);
		}
		packetStatus = ANSERR;
	}
 }
 
 /* ---------------------------------------------------------------------------
  * @brief		читаем длину запроса,
  * @param		ParamSt {packetLen = кол-во символов опред длину пакета
  *				 clientfd = сокет клиента}
  */
 static void fifoSetData(struct ParamSt *param)
 { 
	unsigned int length = 0;
	int flag = -1;
	int count = 0;
	dbgout("SET ->");
	length = readPacketLength(param);
	if((length > 0) & (length <= param->bufSize)) {
		count = bo_recvAllData(param->clientfd, 
				       param->buffer,
			               param->bufSize,
				       length);
		if((count > 0) & (count == length)) flag = 1; 
		else {
			bo_log(" %s fifoSetData() count[%d]!=length[%d]", "FIFO", count, length);
		}
	} else {
		bo_log(" %s fifoSetData() bad length[%d] bufSize[%d]", 
			"FIFO",
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
  * status -> READHEAD |   в случае успешной отправки ждем ответ от клиента
  *             QUIT   |   если возникла ошибка при передаче
  *			   если стек пустой отправляем " NO" и выходим
  */
 static void fifoGetData(struct ParamSt *param)
 {
	int exec = -1, i;
	unsigned char len[2] = {0};
	unsigned char head[3] = "VAL";
	unsigned char headNO[3] = " NO"; 

	
	param->packetLen = bo_getFIFO(param->buffer, param->bufSize);
	boIntToChar(param->packetLen, len);
	
	dbgout("param->packetLen=%d\n", param->packetLen);
	dbgout("FIFO GET buf:");
	for(i = 0; i < param->packetLen; i++) {
		dbgout("%c ", *(param->buffer + i) );
	}
	dbgout("\n");
	 
	if(param->packetLen > 0) {
		exec = bo_sendAllData(param->clientfd, head, 3);
		if(exec == -1){
			bo_log(" %s fifoGetData()send[VAL] errno[%s]", 
				"FIFO",
				strerror(errno));
			goto exit;
		}
		
		exec = bo_sendAllData(param->clientfd, len, 2);
		if(exec == -1) { 
			bo_log(" %s fifoGetData()send[len] errno[%s]", 
				"FIFO",
				strerror(errno));
			goto exit;
		}
		exec = bo_sendAllData(param->clientfd, param->buffer, 
			param->packetLen);
		if(exec == -1) {
			bo_log(" %s fifoGetData()send[data] errno[%s]", 
				"FIFO",
				strerror(errno));
			goto exit;
		}
	} else {
		exec = bo_sendAllData(param->clientfd, headNO, 3);
		if(exec == -1)
			bo_log(" %s fifoGetData()send[NO] errno[%s]", 
				"FIFO",
				strerror(errno));
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
	if(exec == -1) bo_log(" %s fifoAnsOk() errno[%s]", 
				"FIFO", 
				strerror(errno)); 
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
	if(exec == -1) bo_log(" %s fifoAnsNo() errno[%s]", 
				"FIFO",
				strerror(errno));
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
	if(exec == -1) bo_log(" %s fifoAnsErr() errno[%s]", 
		"FIFO",
		strerror(errno));
}
 
/* ---------------------------------------------------------------------------
 * @brief	проверяем сообщение(был ли пакет с данным ID c данного IP)
 *		добавляем в FIFO
 */
static void fifoAddToFIFO(struct ParamSt *param)
{
	int flag = -1;
	int exec = -1;
	const int err = -1;
	int i = 0;
	bo_printFIFO();
	
	if( param->packetLen > 9 ) {
		memset(param->id, 0, 9);
		memcpy(param->id, param->buffer, 8);
		dbgout("FIFO RECV id = [");
		for(i = 0; i < 8; i++) {
			dbgout("%c", *(param->buffer + i) );
		}
		dbgout("] ");
		
		param->buffer += 8;
		param->packetLen -=8;
		
		dbgout("msg[");
		for(i = 0; i < param->packetLen; i++) {
			dbgout("%c", *(param->buffer + i) );
		}
		dbgout("]\n");
		dbgout("From ip[%s]\n", param->ip);
		exec = bo_checkDblMsg(param);
		if(exec == 1) {
			flag = bo_addFIFO(param->buffer, param->packetLen);
			if(flag == -1) {
				dbgout(" ERR WHEN ADD\n");
				bo_log(" %s fifoAddToFIFO() bo_addFIFO can't add data to FIFO bad Length value[%d]",
					"FIFO", param->packetLen);
				goto error;
			} else if(flag == 0) {
				dbgout(" NO ADD. FIFO FULL\n");
				bo_log(" FIFO fifoAddToFIFO() can't add data FIFO is full ");
				goto error;
			}
			exec = bo_chkDbl_setMark(param);
			if(exec == -1) {
				/* del last msg from fifo */
				bo_fifo_delLastAdd();
				goto error;
			}
		} else if(exec == 0) {
			dbgout("\n DOUBLE MESSAGE DOUBLE MESSAGE DOUBLE MESSAGE \n");
			bo_log("FIFO fifoAddToFIFO() value don't push to FIFO");
		} else goto error;
		
	} else {
		bo_log(" %s fifoAddToFIFO() bo_addFIFO can't add data to FIFO bad Length value[%d]",
			"FIFO", param->packetLen);
		goto error;
	}
	
	packetStatus = ANSOK;
	
	if(err == 1) {
		error:
		packetStatus = ANSERR;
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

/* ---------------------------------------------------------------------------
 * @return	[1] Not Double Msg [-1] ERROR [0] Double Msg 
 */
static int bo_checkDblMsg(struct ParamSt *param)
{
	int ans = -1;
	int exec = -1;
	char ip[16] = {0};
	char *tab_id = NULL;
	
	exec = bo_getIp(param->clientfd, ip);
	if(exec == 1) {
	
		tab_id = ht_get(param->id_msg, ip, tab_id);
		if(tab_id != NULL) {
			if(strstr(tab_id, param->id)) {
				dbgout("DOUBLE MSG\n");
				bo_log("bo_checkDblMsg() WARN %s [%s] id[%s][%s]",
					"recv double msg from ip ", 
					ip,
					param->id,
					tab_id);
				ans = 0;
				goto exit;
			}
		}

		ans = 1;
	} else {
		bo_log("bo_checkDblMsg() ERROR can't get ip by sock");
	}
	exit:
	return ans;
}
/* ----------------------------------------------------------------------------
 * @return	[1] OK [-1] ERROR
 */
static int bo_chkDbl_setMark(struct ParamSt *param)
{
	int exec = -1;
	char ip[16] = {0};
	
	exec = bo_getIp(param->clientfd, ip);
	if(exec == 1) {
	
		exec = ht_put(param->id_msg, ip, param->id);
		if(exec == -1) {
			bo_log("bo_chkDbl_setMark() WARN can't add ID to table");
			goto exit;
		}
		exec = 1;
	} else {
		exec = -1;
		bo_log("bo_checkDbl_setMark() ERROR can't get ip by sock");
	}
	
	exit:
	return exec;
}
 /* 0x42 */