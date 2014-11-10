#include "bo_net_master_core.h"

static void coreReadHead(struct paramThr *p);
static void coreSet	(struct paramThr *p);
static void coreQuit    (struct paramThr *p);
static void coreOk	(struct paramThr *p);
static void coreErr     (struct paramThr *p);
static void coreAdd	(struct paramThr *p);
static void coreReadCRC (struct paramThr *p);

/* ----------------------------------------------------------------------------
 *	КОНЕЧНЫЙ АВТОМАТ
 */
static char *coreStatusTxt[] = {"READHEAD", "SET", "QUIT", "ANSOK",
				  "ERR", "ADD", "READCRC"};

static void(*statusTable[])(struct paramThr *) = {
	coreReadHead,
	coreSet,
	coreQuit,
	coreOk,
	coreErr,
	coreAdd,
	coreReadCRC
};

/* ----------------------------------------------------------------------------
 * @brief	принимаем данные от клиента SET - изменения для табл роутов
 *					    LOG - лог команд для устр 485
 * @return	[] тип пришедшего сообщения SET[1]
 *					    LOG[2]
 *					    ERR[-1]
 */
int bo_master_core(struct paramThr *p)
{
	int typeMSG = -1;
	int stop = 1;
	p->status = READHEAD;
	
	while(stop) {
		dbgout("KA[%s]\n", coreStatusTxt[p->status]);
		if(p->status == SET) typeMSG = 1;
		if(p->status == ERR) typeMSG = -1;
		if(p->status == QUIT) break;
		statusTable[p->status](p);
	}
	return typeMSG;
}

/* ----------------------------------------------------------------------------
 * @brief	читаем заголовок
 */
static void coreReadHead(struct paramThr *p)
{
	int exec = -1;
	char buf[3] = {0};
	exec = bo_recvAllData(p->sock, (unsigned char *)buf, 3, 3);
	if(exec == -1) {
		bo_log("bo_net_master_core.c coreReadHead() errno[%s]", 
			strerror(errno));
		p->status = ERR;
	} else {
		if(strstr(buf, "SET")) p->status = SET;
		else if(strstr(buf, "OK"))  p->status = QUIT;
		else p->status = ERR;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	обработка SET|LEN|DATA
 *			  LEN - 2 byte
 *			  DATA - XXX:XXX.XXX.XXX.XXX:XXX
 */
static void coreSet(struct paramThr *p)
{
	int length = 0;
	int flag  = -1;
	int count = 0;
	length = bo_readPacketLength(p->sock);
	/* длина сообщения минимум 10 байта = 5(XXX:VVVV) байт информации + 2 байта CRC */
	memset(p->buf, 0, p->bufSize);
	if((length > 9) & (length <= p->bufSize)) {
		count = bo_recvAllData(p->sock, 
				       p->buf,
			               p->bufSize,
				       length);
		if((count > 0) & (count == length)) flag = 1; 
		else {
			bo_log("bo_net_master_core.c coreSet() count[%d]!=length[%d]", count, length);
			bo_log("bo_net_master_core.c coreSet() buf[%s]", p->buf);
		}
	} else {
		bo_log("bo_net_master_core.c coreSet() bad length[%d] ", 
			length, 
			p->bufSize);
	}
	if(flag == 1) {
		p->length = length;
		p->status = READCRC;
	} else {
		p->status = ERR;
	}
}

static void coreReadCRC(struct paramThr *p)
{
	int crc = -1;
	int count = -1;
	unsigned char crcTxt[2] = {0};
	char *msg = (char *)p->buf;
	int msg_len = p->length - 2;
	int i = 0;
	crcTxt[0] = p->buf[p->length - 2];
	crcTxt[1] = p->buf[p->length - 1];
	
	crc = boCharToInt(crcTxt);
	printf("crc[%02x %02x] = [%d]\n", crcTxt[0], crcTxt[1], crc); 
	count = crc16modbus(msg, msg_len);
	printf("count[%d]\n msg[", count);
	
	for(; i < msg_len; i++) {
		printf("%c", msg[i]);
	}
	printf("]\n");
	if(crc != count) p->status = ERR;
	else p->status = ADD;
}

static void coreAdd(struct paramThr *p) 
{
	char addr485[4] = {0};
	char value[18] = {0};
	int exec = 0;
	memcpy(addr485, p->buf, 3);
	memcpy(value, (p->buf + 4), 17);
/*	
	dbgout("coreAdd() ADD ");
	printf("\naddr485[");
	
	for(; i < sizeof(addr485); i++) {
		printf("%c", addr485[i]);
	}
	printf("]\nvalue:[");
	for(i = 0; i < sizeof(value); i++) {
		printf("%c", value[i]);
	}
	printf("]\n");
*/	
	/* value должен быть строкой обяз-но!!! тк функция ht_put() принимает 
	 * в качестве параметра строку*/
	value[17] = '\0';
	exec = ht_put(p->route_tab, addr485, value);
	if(exec == 0) p->status = ANSOK;
	else {
		bo_log("coreAdd() can't add key[%s] value[%s] from ht_put()",
			addr485, value);
		p->status = ERR;
	} 
}

static void coreQuit(struct paramThr *p) { }

static void coreErr(struct paramThr *p)  
{ 
	int exec = -1;
	unsigned char msg[] = "ERR";

	exec = bo_sendAllData(p->sock, msg, 3);
	if(exec == -1) bo_log("coreErr() errno[%s]", strerror(errno));
	p->status = QUIT;
}

static void coreOk(struct paramThr *p) 
{ 
	int exec = -1;
	unsigned char msg[] = " OK";
	exec = bo_sendAllData(p->sock, msg, 3);
	if(exec == -1) bo_log("coreOk() errno[%s]", strerror(errno));
	p->status = QUIT;
}

