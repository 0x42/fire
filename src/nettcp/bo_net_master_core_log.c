#include "bo_net_master_core_log.h"

static void logReadHead	(struct KA_log_param *);
static void logLog	(struct KA_log_param *);
static void logReadCrc	(struct KA_log_param *);
static void logOk	(struct KA_log_param *);
static void logErr	(struct KA_log_param *);
static void logQuit	(struct KA_log_param *);
static void logNul	(struct KA_log_param *);
static char *ka_logStatusTxt[] = {"LOG READHEAD", "LOG LOG", "LOG READCRC", 
				  "LOG OK", "LOG ERR", "LOG QUIT", "LOG NULL"};

static void(*KA_Table[])(struct KA_log_param *) = {
	logReadHead,
	logLog,
	logReadCrc,
	logOk,
	logErr,
	logQuit,
	logNul
};
/* ----------------------------------------------------------------------------
 * @brief	<= LOG|LEN|DATA|CRC
 *		DATA write in @buf
 * 	     OK =>
 * @return  [-1] ERROR [>0] - length log [0] - нет лога по такому индексу
 */
int bo_master_core_log(int sock, char *buf, int bufSize)
{
	struct KA_log_param ka_p;
	ka_p.sock = sock;
	ka_p.buf = buf;
	ka_p.bufSize = bufSize;
	ka_p.len = -1;
	ka_p.status = LOGREADHEAD;
	
	while(1) {
		dbgout("bo_master_core_log [%s]\n", ka_logStatusTxt[ka_p.status]);
		if(ka_p.status == LOGQUIT) break;
		if(ka_p.status == LOGNUL) { 
			ka_p.len = 0;
			break;
		}
		if(ka_p.status == LOGERR) {
			ka_p.len = -1;
			break;
		}
		KA_Table[ka_p.status](&ka_p);
	}
	return ka_p.len;
}

static void logReadHead(struct KA_log_param *p)
{
	int exec = -1;
	char buf[3] = {0};
	
	exec = bo_recvAllData(p->sock, (unsigned char *)buf, 3, 3);
	if(exec == -1) {
		bo_log("bo_net_master_core_log.c logReadHead() errno[%s]", 
			strerror(errno));
		p->status = LOGERR;
	} else {
		if(strstr(buf, "LOG")) p->status = LOGLOG;
		else if(strstr(buf, "NUL")) p->status = LOGNUL;
		else p->status = LOGERR;
	}
}

static void logLog(struct KA_log_param *p)
{
	int length = 0;
	int flag  = -1;
	int count = 0;
	length = bo_readPacketLength(p->sock);
	/*LOG|LEN|DATA(HEADER=10 + VALUE)  + CRC*/
	memset(p->buf, 0, p->bufSize);
	if((length > 10) & (length <= p->bufSize)) {
		count = bo_recvAllData(p->sock, 
				       (unsigned char *)p->buf,
			               p->bufSize,
				       length);
		if((count > 0) & (count == length)) flag = 1; 
		else {
			bo_log("bo_net_master_core_log.c logLog() count[%d]!=length[%d]", count, length);
			bo_log("bo_net_master_core_log.c logLog() buf[%s]", p->buf);
		}
	} else {
		bo_log("bo_net_master_core_log.c logLog() bad length[%d] ", 
			length, 
			p->bufSize);
	}
	if(flag == 1) {
		p->len = length;
		p->status = LOGREADCRC;
	} else {
		p->status = LOGERR;
	}
}

static void logReadCrc(struct KA_log_param *p) 
{
	int crc = -1;
	int count = -1;
	unsigned char crcTxt[2] = {0};
	char *msg = (char *)p->buf;
	int msg_len = p->len - 2;
	
	crcTxt[0] = p->buf[p->len - 2];
	crcTxt[1] = p->buf[p->len - 1];
	
	crc = boCharToInt(crcTxt);
	count = crc16modbus(msg, msg_len);

	p->len = msg_len;
	
	if(crc != count) { 
		p->status = LOGERR;
		p->len = -1;
		bo_log("logReadCrc() bad CRC");
	} else p->status = LOGOK;
}

static void logOk(struct KA_log_param *p) 
{
	int exec = -1;
	unsigned char msg[] = " OK";
	int i = 0;
	dbgout("len[%d]\nlog[", p->len);
	for(; i < p->len; i++) {
		dbgout("%c", *(p->buf + i) );
	}
	dbgout("]\n");
	
	exec = bo_sendAllData(p->sock, msg, 3);
	if(exec == -1) { 
		bo_log("logOk() errno[%s]", strerror(errno));
		p->len = -1;
		p->status = LOGERR;
	} else {
		p->status = LOGQUIT;
	}
}

static void logErr(struct KA_log_param *p) {}

static void logQuit(struct KA_log_param *p) {}

static void logNul(struct KA_log_param *p) {}

/* ---------------------------- END KA ---------------------------------------*/

/*----------------------------------------------------------------------------
 * @brief	отправляет запрос 1) RLO|index => master
 *		принимает лог	  2)          <= LOG
 *		сохраняет LOG в @buf
 * @buf		
 * @index	номер лога 
 * @return	[-1] Error [0] haven't got log [>0] log length
 */
int bo_master_core_logRecv(int sock, int index, char *buf, int bufSize) 
{
	int exec = -1;
	exec = bo_sendRloMsg(sock, index);
	if(exec > 0) {
		exec = bo_master_core_log(sock, buf, bufSize);
		if(exec == -1)		
			bo_log("bo_master_core_log -> bo_sendRloMsg ERROR");
	} else {
		bo_log("bo_master_core_logRecv -> bo_sendRloMsg ERROR");
	}
	return exec;
}

/* 0x42 */