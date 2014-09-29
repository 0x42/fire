#include "../tools/dbgout.h"
#include "../log/bologging.h"
#include "bo_net.h"

struct ParamSt;
static int readConfig();
static int fifoServStart();
static void fifoServWork(int sockfdMain);
static void fifoReadPacket();
static void fifoReadHead(struct ParamSt *param);
static void fifoError(struct ParamSt *param);
static void fifoQuit(struct ParamSt *param);
static void fifoSetData(struct ParamSt *param);
static void fifoGetData(struct ParamSt *param);
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
	bo_log("%s%s %d", " INFO ", " START LOOP", sockfdMain);
	while(stop == 1) {
		if(bo_waitConnect(sockfdMain, &fifoconf.clientfd, &errTxt) == 1) {
			countErr = 0;
			fifoReadPacket();
			close(fifoconf.clientfd);
			fifoconf.clientfd = 0;
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
	struct ParamSt param;
	packetStatus = READHEAD;
	int stop = -1;
	while(stop == -1) {
		printf("status = %s\n", PacketStatusTxt[packetStatus]);
		if(packetStatus == QUIT) break;
		statusTable[packetStatus](&param);
	}
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
	char buf[headSize];
	int  count = 0;
	int exec = 0;
	/*  в течение 1 сек ждем прихода headSize байт */
	while(exec != 10) {
		/* MSG_PEEK после чтения данных с сокета их копия 
		 * остается в очереди если на сокет не придет не одного байта то 
		 * заблокируемся надолго */
		count = recv(param->clientfd, buf, headSize, MSG_PEEK);
		if(count == 3)  break;
		usleep(100000);
		exec++;
	}
	memset(buf, 0, headSize);
	count = recv(param->clientfd, buf, headSize, 0);
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
	 int length = 0;
	printf("fifoSetData()\n");
	length = readPacketLength(param);
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