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

static int sendValCrc(struct ParamSt *param, char *value, int length);
static int sendHeadLen(struct ParamSt *param, int length);

static void routeReadHead	(struct ParamSt *param);
static void routeSet		(struct ParamSt *param);
static void routeGet		(struct ParamSt *param);
static void routeQuit		(struct ParamSt *param);
static void routeAnsOk		(struct ParamSt *param);
static void routeAnsErr		(struct ParamSt *param);
static void routeAnsNo		(struct ParamSt *param);
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
static char *PacketStatusTxt[] = {"READHEAD", "SET", "GET", "QUIT", "ANSOK",
				  "ANSERR", "ANSNO", "ADDTAB", "READCRC"};

static enum PacketStatus {READHEAD = 0, SET, GET, QUIT, ANSOK, ANSERR, ANSNO,
			ADDTAB, READCRC} packetStatus;

static void(*statusTable[])(struct ParamSt *) = {
	routeReadHead,
	routeSet,
	routeGet,
	routeQuit,
	routeAnsOk,
	routeAnsErr,
	routeAnsNo,
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
	/* длина сообщения минимум 7 байта = 5(XXX:V) байт информации + 2 байта CRC */
	if((length > 6) & (length <= param->bufSize)) {
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

/* ----------------------------------------------------------------------------
 * @brief	читаем ADDR485(3bytes) ищем в таблице по Addr485 и отправл VAL
 *		"GET"   ->
 *		ADDR485 ->
 *			<- "VAL"
 *			<- LENGTH
 *			<- VALUE
 *			<- CRC
 * @status      KA -> ANSERR | READHEAD
 */
static void routeGet(struct ParamSt *param)
{
	int exec = -1;
	unsigned char addr485[3] = {0};
	char *value = NULL;
	int count = -1;
	int length = 0 ;
	dbgout("routeGet() GET ");
	/* читаем адрес 485 */
	count = bo_recvAllData(param->clientfd, addr485, 3, 3);
	if(count == 3) {
		value = ht_get(tabRoutes, (char *)addr485, NULL);
		if(value == NULL)
			packetStatus = ANSNO;
		else {
			length = strlen(value);
			exec = sendHeadLen(param, length);
			if(exec == -1) goto error;
			exec = sendValCrc(param, value, length);
			if(exec == -1) goto error;
			exec = 1;
		}
	} else {
		bo_log("routeGet() ERROR can't read addr485 errno[%s]", 
			strerror(errno));
		goto error;
	}
	
	if(exec == -1) {
		error:
		packetStatus = ANSERR;
	} else {
		packetStatus = READHEAD;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	отправ " OK"(3bytes) KA -> QUIT
 */
static void routeAnsOk(struct ParamSt *param)
{
	int exec = -1;
	int sock = param->clientfd;
	unsigned char msg[] = " OK";
	dbgout("routeAnsOk() ANSOK ");
	exec = bo_sendAllData(sock, msg, 3);
	if(exec == -1) bo_log("routeAnsOk() errno[%s]", strerror(errno));
	packetStatus = QUIT;
}

/* ----------------------------------------------------------------------------
 * @brief	отправ "ERR"(3bytes) KA -> QUIT
 */
static void routeAnsErr(struct ParamSt *param)
{
	dbgout("routeAnsErr() ANSERR ");
	int exec = -1;
	int sock = param->clientfd;
	unsigned char msg[] = "ERR";
	exec = bo_sendAllData(sock, msg, 3);
	if(exec == -1) bo_log("routeAnsErr() errno[%s]", strerror(errno));
	packetStatus = QUIT;
}

/* ----------------------------------------------------------------------------
 * @brief	отправ " NO"(3bytes) KA -> QUIT
 */
static void routeAnsNo(struct ParamSt *param)
{
	int exec = -1;
	int sock = param->clientfd;
	unsigned char msg[] = " NO";
	dbgout("routeAnsNo() ANSNO ");
	exec = bo_sendAllData(sock, msg, 3);
	if(exec == -1) bo_log("routeAnsNo() errno[%s]", strerror(errno));
	packetStatus = QUIT;
}

/* ----------------------------------------------------------------------------
 * @brief	измен значение в таблице tabRoutes 
 *		packet = [123:127.0.0.1:2XX] max size = 21 + 2(CRC) 
 *		123       - addr485 (3bytes)
 *		127.0.0.1 - ip moxa (XXX.XXX.XXX.XXX) 15
 *		2         - port485
 *		XX	  - CRC (2bytes)
 */
static void routeAddTab(struct ParamSt *param)
{
	char addr485[4] = {0};
	char value[18] = {0};
	int i = 0;
	int exec = 0;
	memcpy(addr485, param->buffer, 3);
	memcpy(value, (param->buffer + 4), 17);
	dbgout("routeAddTab() ADD_TAB ");
	printf("\naddr485[");
	for(; i < sizeof(addr485); i++) {
		printf("%c",addr485[i]);
	}
	printf("]\nvalue:[");
	for(i = 0; i < sizeof(value); i++) {
		printf("%c", value[i]);
	}
	printf("]\n");
	/* value должен быть строкой обяз-но!!! тк функция ht_put() принимает 
	 * в качестве параметра строку*/
	value[17] = '\0';
	exec = ht_put(tabRoutes, addr485, value);
	if(exec == 0) packetStatus = ANSOK;
	else {
		bo_log("routeAddTab() can't add key[%s] value[%s] from ht_put()",
			addr485, value);
		packetStatus = ANSERR;
	}
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
	crcTxt[0] = param->buffer[param->packetLen - 2];
	crcTxt[1] = param->buffer[param->packetLen - 1];
	
	crc = boCharToInt(crcTxt);
/*	printf("crc[%02x %02x] = [%d]", crcTxt[0], crcTxt[1], crc); */
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

/* ----------------------------------------------------------------------------
 * @brief	отправ "VAL" + LENGTH(2bytes). Ошибки пишем в лог
 * @return	[-1] error	[1] - ok
 */
static int sendHeadLen(struct ParamSt *param, int length)
{
	int  ans = -1;
	unsigned char head[3] = "VAL";
	unsigned char len[2] = {0};
	int exec = -1;
	
	boIntToChar(length, len);
	
	exec = bo_sendAllData(param->clientfd, head, 3);
	if(exec == -1){
		bo_log("sendHeadLen()send[VAL] errno[%s]", 
			strerror(errno));
		goto exit;
	}

	exec = bo_sendAllData(param->clientfd, len, 2);
	if(exec == -1){
		bo_log("sendHeadLen()send[len] errno[%s]", 
			strerror(errno));
		goto exit;
	}
	ans = 1;
	exit:
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	отпр Value и CRC
 * @return	[-1] Error [1] Ok
 */
static int sendValCrc(struct ParamSt *param, char *value, int length)
{
	int ans = -1;
	int crc = -1;
	int exec = -1;
	unsigned char crcTxt[2] = {0};
	
	crc = crc16modbus(value, length);
	boIntToChar(crc, crcTxt);
	
	exec = bo_sendAllData(param->clientfd, (unsigned char *)value, length);
	if(exec == -1) {
		bo_log("sendValCrc() ERROR send[%s] errno[%s]", 
			value, strerror(errno));
		goto exit;
	}
	
	exec = bo_sendAllData(param->clientfd, (unsigned char *)value, length);
	if(exec == -1) {
		bo_log("sendValCrc() ERROR send[crc] errno[%s]", strerror(errno));
		goto exit;
	}
	ans = 1;
	exit:
	return ans;
}













