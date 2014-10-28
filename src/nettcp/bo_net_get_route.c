/* ----------------------------------------------------------------------------
 * @brief	отправляем GET ->
 *			ADDR485->
 *			       <- VAL|NO
 *			          if send VAL	      	
 *			       <- LENGTH(2bytes)
 *			       <- DATA
 *			       <- CRC
 * @return	[>0] - length value 
 *		[-1] Error 
 *		[0] no value in table 
 */
#include "bo_net.h"
#include "../tools/ocrc.h"

static struct Param {
	int sock;
	char *buf;
	int bufSize;
	int length;
	char *addr485;
} param;

static int checkCRC(unsigned char *crcTxt, char *msg, int msg_len);

static void recvStart		(struct Param *p);
static void recvReadHead	(struct Param *p);
static void recvVal		(struct Param *p);
static void recvNo		(struct Param *p);
static void recvErr		(struct Param *p);
static void recvEnd		(struct Param *p);
/*
static char *statusTxt[] = {"START", "READHEAD", "VAL", "NO", "ERR", "END"};
*/
static enum Status {START=0, READHEAD, VAL, NO, ERR, END} status;

static void (*statusTable[])(struct Param *) = {
	recvStart,
	recvReadHead,
	recvVal,
	recvNo,
	recvErr,
	recvEnd
};

/* ----------------------------------------------------------------------------
 * @brief		возвращает данные и очереди
 * @addr485		адрес RS485 3 bytes
 * @buf			значения из таблицы
 * @bufSize		размер буфера
 * @return		[0] - в таблице нет значений 
 *			[-1] error 
 *			[>0] размер скопир данных в буфер			 
 */
int bo_recvRoute(char *ip, 
		 unsigned int port, 
		 char *addr485, 
	         char *buf, 
	         int bufSize)
{
	int ans  = -1;
	int stop = 1;
	int sock = -1;
	sock = bo_setConnect(ip, port);
	if(sock > 0) {
		param.sock    = sock;
		param.buf     = buf;
		param.bufSize = bufSize;
		param.length  = 0;
		param.addr485 = addr485;
		status = START;
		while(stop) {
			if(status == NO) {
				ans = 0;
				break;
			}
			if(status == END) {
				ans = param.length;
				break;
			}
			if(status == ERR) {
				ans = -1; 
				break;
			}
			statusTable[status](&param);
		}
		if(close(sock)==-1) {
			bo_log("bo_recvRoute() when close socket errno[%s]\n ip[%s]\nport[%d]\n", 
				strerror(errno), ip, port);
		}
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	отправ "GET" + ADDR485(p->addr485)
 * @status	KA -> READHEAD | ERR
*/
static void recvStart(struct Param *p) 
{
	unsigned char head[] = "GET";
	int exec = -1;
	
	exec = bo_sendAllData(p->sock, head, 3);
	if(exec == -1) {
		bo_log("bo_net_get_route.c recvStart() send[GET] errno[%s]", 
			strerror(errno));
		goto error;
	}
	
	exec = bo_sendAllData(p->sock, (unsigned char *)p->addr485, 3);
	if(exec == -1) {
		bo_log("bo_net_get_route.c recvStart() send[ADDR485]errno[%s]", 
			strerror(errno));
		goto error;
	}
	
	if(exec == -1) {
		error:
		status = ERR;
	} else {
		status = READHEAD;
	}
}

static void recvReadHead(struct Param *p)
{	
	int exec = -1;
	char buf[4] = {0};
	exec = bo_recvAllData(p->sock, (unsigned char *)buf, 3, 3);
	if(exec == -1) {
		bo_log("bo_net_get_route.c recvReadHead() errno[%s]", 
			strerror(errno));
		status = ERR;
	} else {
		if(strstr(buf, "VAL"))     status = VAL;
		else if(strstr(buf, "NO")) status = NO;
		else if(strstr(buf, "OK")) status = END;
		else status = ERR;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	читаем значение Length далее читаем данные размером length
 *		AK -> OK(все данные получили ) | ERR (ошибка) 
 */
static void recvVal(struct Param *p)
{
	const int error = -1;
	int exec = -1;
	int length = 0;
	unsigned char len[2] = {0};
	unsigned char crcTxt[2] = {0};
	exec = bo_recvAllData(p->sock, len, 2, 2);
	
	if(exec == -1) goto error;
	else {
		length = boCharToInt(len);
		if(length <= p->bufSize) {
			memset(p->buf, 0, p->bufSize);
			exec = bo_recvAllData(p->sock, (unsigned char *)p->buf, p->bufSize, length);
			if(exec == -1) goto error;
			
			exec = bo_recvAllData(p->sock, crcTxt, 2, 2);
			if(exec == -1) goto error;
			
			exec = checkCRC(crcTxt, p->buf, length);
			if(exec == -1) {
				bo_log("bo_net_get_route.c recvVal() incorrect CRC");
				goto error;
			} else p->length = length;
		} else {
			bo_log("bo_net_get_route.c recvVal() bad length[%d] bufSize=[%d]",
				length, p->bufSize);
			goto error;
		}
	}
	status = END;
	if(error == 1) {
		error:
		bo_log("bo_net_get_route.c recvVal() errno[%s]", strerror(errno));
		status = ERR;
	}
}

static void recvEnd(struct Param *param) {}
static void recvNo (struct Param *param) {}
static void recvErr(struct Param *param) {}
/* ----------------------------------------------------------------------------
 * @return [1] == [-1] !=
 */

static int checkCRC(unsigned char *crcTxt, char *buf, int len)
{
	int ans = -1;
	int crc = -1;
	int crc_count = -1;

	crc = boCharToInt(crcTxt);
	crc_count = crc16modbus(buf, len);
	if(crc != crc_count) ans = -1;
	else ans = 1;
	
	return ans;
}

/* 0x42 */