#include <stdlib.h>
#include <string.h>
#include "../tools/dbgout.h"
#include "../log/bologging.h"
#include "bo_net.h"
#include "../tools/ocrc.h"
#include "../tools/ocfg.h"

struct ParamSt;

static void readConfig(TOHT *cfg, int n, char **argv);
static unsigned int readPacketLength(struct ParamSt *param);
static void routeServWork(int sockMain);
static void routeReadPacket(int clientSock, unsigned char *buffer, int bufSize,
			int *endPr);

static void routeReadHead	(struct ParamSt *param);
static void routeSet		(struct ParamSt *param);
static void routeGet		(struct ParamSt *param);
static void routeQuit		(struct ParamSt *param);
static void routeAnsErr		(struct ParamSt *param);
static void routeAddTab		(struct ParamSt *param);
static void routeReadCRC	(struct ParamSt *param);
/* ----------------------------------------------------------------------------
 * @port	- порт на котором слушает сервер
 * @queue_len	- очередь запросов ожид-х обраб сервером
 */
static struct {
	int port;
	int queue_len;
} servconf = {0};

static TOHT *tabRoutes = NULL;

/* @packetLen		длина данных
 * @clientfd		сокет 
 * @buffer		буфер с данными
 * @bufSize		размер буфера
 */
struct ParamSt {
	int packetLen;
	int clientfd;
	unsigned char *buffer;
	int bufSize;
};

/* ----------------------------------------------------------------------------
 * @brief	
 */
static char *PacketStatusTxt[] = {"READHEAD", "SET", "GET", "QUIT", "ANSERR", 
				  "ADDTAB", "READCRC"};

static enum PacketStatus {READHEAD = 0, SET, GET, QUIT, ANSERR, ADDTAB,
			  READCRC} packetStatus;

static void(*statusTable[])(struct ParamSt *) = {
	routeReadHead,
	routeSet,
	routeGet,
	routeQuit,
	routeAnsErr,
	routeAddTab,
	routeReadCRC
};
/* ----------------------------------------------------------------------------
 * @brief		Запуск сервера ROUTE(хран глоб. табл адресов 
 *			ip, addr485, port485).
 */
void bo_route_main(int n, char **argv)
{
	int sock = 0;
	TOHT *cfg = NULL;
	
	readConfig(cfg, n, argv);
	bo_log("%s%s", " INFO ", "START moxa_route");
	tabRoutes = ht_new(255);
	if(tabRoutes == NULL) {
		bo_log("%s%s", " ERROR ", "can't create hash table tabRoutes");
	} else {
		/* иниц-ия табл. для вычисл. контр. суммы*/
		gen_tbl_crc16modbus();
		if( (sock = bo_servStart(servconf.port, servconf.queue_len)) != -1) {
			routeServWork(sock);
			bo_closeSocket(sock);
		}
	}
	if(tabRoutes != NULL) {
		ht_free(tabRoutes);
		tabRoutes = NULL;
	}
	if(cfg != NULL) {
		cfg_free(cfg);
		cfg = NULL;
	}
	bo_log("%s%s", " INFO ", "END   moxa_route");
}

static void readConfig(TOHT *cfg, int n, char **argv)
{
	int defP = 8889, defQ = 20;
	char *fileName	= NULL;
	char *f_log	= "moxa_route.log";
	char *f_log_old = "moxa_route.log(old)";
	int  nrow   = 0;
	int  maxrow = 1000;
	servconf.port      = defP;
	servconf.queue_len = defQ;
	
	if(n == 2) {
		fileName = *(argv + 1);
		cfg = cfg_load(fileName);
		if(cfg != NULL) {
			servconf.port      = cfg_getint(cfg, "sock:port", defP);
			servconf.queue_len = cfg_getint(cfg, "sock:queue_len", defQ);
			
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
}

/* ----------------------------------------------------------------------------
 * @brief		осн цикл сервера. вход соед и отодает его worker'у
 * @sock_serv		дескриптор главного сокета на который поступают
 *			соединения. Не закрывать его!!!
 */
static void routeServWork(int sock_serv)
{
	int stop = 1;
	int countErr  = 0;
	char *errTxt = NULL;
	int sock_client = 0;

	int bufSize = 128;
	unsigned char *buf1 = NULL;

	bo_log("%s%s", " INFO ", " RUN ");

	/* создаем буфер */
	buf1 = (unsigned char *)malloc(bufSize);
	if(buf1 == NULL) {
		bo_log("%s%s", " ERROR ", "routeServWork() can't create buf1");
		return;
	}

	while(stop == 1) {
		if(bo_waitConnect(sock_serv, &sock_client, &errTxt) == 1) {
			countErr = 0;
			/* передаем флаг stop. Ф-ия может поменять его на -1
			   если придет пакет END */
			routeReadPacket(sock_client, buf1, bufSize, &stop);
		} else {
			countErr++;
			bo_log("%s%s errno[%s]", " ERROR ", 
				"routeServWork()", errTxt);
			if(countErr == 10) stop = -1;
		}
	}
	free(buf1);
}

/* ---------------------------------------------------------------------------
  * @brief		
  * @clientSock		дескриптор сокета(клиента) При завершение сокет закр-ся.
  * @buffer		буфер в который пишем данные. 
  * @bufSize		размер буфера
  * @endPr		флаг прекращ работы сервера и заверш программы
  */
static void routeReadPacket(int clientSock, unsigned char *buffer, int bufSize,
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
	 * порт, иначе придется ждать 2MSL(особенности протокола TCP) */
	setsockopt(clientSock, SOL_SOCKET, SO_REUSEADDR, &i, sizeof(i));
	dbgout("\n> ----------	CONNECT ---------------- <\n");
	while(stop == -1) {
		dbgout("\n>>>AVTOMAT STATUS = %s\n", PacketStatusTxt[packetStatus]);
		if(packetStatus == QUIT) break;
		statusTable[packetStatus](&param);
	}
	bo_closeSocket(clientSock);
	dbgout("\n> ----------- END CONNECT ------------ <\n");
}

/* ----------------------------------------------------------------------------
 * @brief		читаем заголовок пакета
 */
static void routeReadHead(struct ParamSt *param)
{
	int bufSize = 3;
	char buf[bufSize + 1];
	int exec = 0;
	buf[bufSize] = '\0';
	exec = bo_recvAllData(param->clientfd, (unsigned char *)buf, bufSize, 3);
	if(exec == -1) {
		bo_log("routeReadHead() errno[%s]", strerror(errno));
	}
	if(exec == 3) {
		dbgout("routeReadHead()->HEAD[%s] exec=%d", buf, exec);
		if(strstr(buf, "SET")) packetStatus = SET;
		else if(strstr(buf, "GET")) packetStatus = GET;
		else if(strstr(buf, "END")) packetStatus = QUIT;
		else packetStatus = ANSERR;
	} else {
		if(exec > 0 ) dbgout("routeReadHead()->HEAD[%s] exec=%d", buf, exec);
		packetStatus = ANSERR;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	читаем длину запроса, его тело и кладем в ТАB_ROUTE
 * @param	ParamSt {packetLen = кол-во символов опред длину пакета
 *				 clientfd = сокет клиента}
 */
static void routeSet(struct ParamSt *param)
{
	unsigned int length = 0;
	int flag = -1;
	int count = 0;
	length = readPacketLength(param);
	/* длина сообщения минимум 3 байта = 1 байт информации + 2 байта CRC */
	if((length > 3) & (length <= param->bufSize)) {
		count = bo_recvAllData(param->clientfd, 
				       param->buffer,
			               param->bufSize,
				       length);
		if((count > 0) & (count == length)) flag = 1; 
		else {
			bo_log("routeSet() count[%d]!=length[%d]", count, length);
		}
	} else {
		bo_log("routeSet() bad length[%d] ", 
			length, 
			param->bufSize);
	}
	if(flag == 1) {
		param->packetLen = length;
		packetStatus = READCRC;
	} else {
		packetStatus = ANSERR;
	}
}

static void routeGet(struct ParamSt *param)
{
	dbgout("routeGet() GET ");
	packetStatus = QUIT;
}

static void routeAnsErr(struct ParamSt *param)
{
	dbgout("routeAnsErr() ANSERR ");
	packetStatus = QUIT;
}

/* ----------------------------------------------------------------------------
 * @brief	измен значение в таблице tabRoutes 
 */
static void routeAddTab(struct ParamSt *param)
{
	dbgout("routeAddTab() ADD_TAB ");
	
	packetStatus = QUIT;
}

/* ----------------------------------------------------------------------------
 * @brief	проверка контрол суммы(послед 2 байта в сообщении)
 * @result      change packetStatus = ANSERR | ADD_TAB	
 */
static void routeReadCRC(struct ParamSt *param)
{
	int crc = -1;
	int count = -1;
	unsigned char crcTxt[2] = {0};
	char *msg = (char *)param->buffer;
	int msg_len = param->packetLen - 2;
	crcTxt[0] = param->buffer[param->packetLen - 1];
	crcTxt[1] = param->buffer[param->packetLen];
	crc = boCharToInt(crcTxt);
	count = crc16modbus(msg, msg_len);
	if(crc != count) packetStatus = ANSERR;
	else packetStatus = ADDTAB;
}

static void routeQuit(struct ParamSt *param) {}
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
