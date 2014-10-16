/* ----------------------------------------------------------------------------
 * @brief отправляем GET ->
 *			 <- VAL|NO
 *	  NO = return 0;
 *	  VAL:			 
 *		         <- LENGTH(2bytes)
 *			 <- DATA[LENGTH]	 
 *		     DEL ->
 *                       <- OK
 *	  if(OK) return LENGTH
 *        else   return -1
 */
#include "bo_net.h"

static struct Param {
	int  sock;
	unsigned char *buf;
	int  bufSize;
	int  length;
} param;

static void recvStart   (struct Param *p);
static void recvReadHead(struct Param *p);
static void recvVal     (struct Param *p);
static void recvDel     (struct Param *p);
static void recvNo	(struct Param *p);
static void recvErr     (struct Param *p);
static void recvEnd     (struct Param *p);

static int setConnect(char *ip, int port);

/* Возм - ые состояния КА */
static enum Status {START=0, READHEAD, VAL, DEL, NO, ERR, END} status;

/* Ф - ии реал - ие КА */
static void(*statusTable[])(struct Param *) = {
	recvStart,
	recvReadHead,
	recvVal,
	recvDel,
	recvNo,
	recvErr,
	recvEnd
};

int bo_recvDataFIFO(char *ip, unsigned int port, unsigned char *buf, int bufSize)
{
	int ans  = -1;
	int stop = 1;
	int sock = -1;
	sock = setConnect(ip, port);
	if(sock > 0) {
		param.sock    = sock;
		param.buf     = buf;
		param.bufSize = bufSize;
		param.length  = 0;
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
		close(sock);
	}
	return ans;
}

static void recvStart(struct Param *p) 
{
	unsigned char head[] = "GET";
	int exec = -1;
	exec = bo_sendAllData(p->sock, head, 3);
	if(exec == -1) {
		bo_log("bo_net_get.c recvStart() errno[%s]", 
			strerror(errno));
		status = ERR;
	} else {
		status = READHEAD;
	}
}

static void recvReadHead(struct Param *p)
{	
	int exec = -1;
	char buf[3] = {0};
	exec = bo_recvAllData(p->sock, (unsigned char *)buf, 3, 3);
	if(exec == -1) {
		bo_log("bo_net_get.c recvReadHead() errno[%s]", 
			strerror(errno));
		status = ERR;
	} else {
		if(strstr(buf, "VAL"))     status = VAL;
		else if(strstr(buf, "NO")) status = NO;
		else if(strstr(buf, "OK")) status = END;
		else status = ERR;
	}
}

static void recvVal(struct Param *p)
{
	int error = -1;
	int exec = -1;
	int length = 0;
	unsigned char len[2] = {0};
	exec = bo_recvAllData(p->sock, len, 2, 2);
	if(exec == -1) goto error;
	else {
		length = boCharToInt(len);
		if(length <= p->bufSize) {
			exec = bo_recvAllData(p->sock, p->buf, p->bufSize, length);
			if(exec == -1) goto error;
			else p->length = length;
		} else {
			bo_log("bo_net_get.c recvVal() length=%d bufSize=%d",
				length, p->bufSize);
			goto error;
		}
	}
	status = DEL;
	if(error == 1) {
		error:
		bo_log("bo_net_get.c recvVal() errno[%s]", strerror(errno));
		status = ERR;
	}
}

static void recvDel (struct Param *p)
{
	int exec = -1;
	char head[] = "DEL";
	exec = bo_sendAllData(p->sock, (unsigned char*)head, 3);
	if(exec == -1) status = ERR;
	else status = READHEAD;
}

static void recvNo  (struct Param *p) {}
static void recvErr (struct Param *p) {}
static void recvEnd (struct Param *p) {}
/* ----------------------------------------------------------------------------
 * @brief	устан соед-ие
 * @return      [-1] error; [>0] sock 
 */
static int setConnect(char *ip, int port)
{
	int sock = -1;
	struct sockaddr_in saddr;
	sock = bo_crtSock(ip, port, &saddr);
	if(sock > 0) {
		bo_setTimerRcv(sock);
		if(connect(sock, 
			   (struct sockaddr *)&saddr, 
			   sizeof(struct sockaddr)) != 0) {
			bo_log("bo_net_get.c->setConnect() errno[%s]\n ip=%s \
				port=%d",
				strerror(errno),
				ip,
				port);
			close(sock);
			sock = -1;
		}
		
	}
	return sock;
}

/* 0x42 */