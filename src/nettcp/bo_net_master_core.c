#include "bo_net_master_core.h"

STATIC int checkCRC(struct paramThr *p);

static void coreReadHead    (struct paramThr *p);
static void coreSet	    (struct paramThr *p);
static void coreQuit        (struct paramThr *p);
static void coreOk	    (struct paramThr *p);
static void coreErr         (struct paramThr *p);
static void coreAdd	    (struct paramThr *p);
static void coreReadCRC     (struct paramThr *p);
static void coreTab	    (struct paramThr *p);
static void coreReadCRC_Tab (struct paramThr *p);
STATIC void coreReadRow     (struct paramThr *p);
static void coreLog	    (struct paramThr *p);
static void coreSaveLog     (struct paramThr *p);
static void coreReturnLog   (struct paramThr *p);
static void coreSendLog	    (struct paramThr *p);
static void coreSendNul	    (struct paramThr *p);
static void coreAsk	    (struct paramThr *p);
/* ----------------------------------------------------------------------------
 *	КОНЕЧНЫЙ АВТОМАТ
 */
static char *coreStatusTxt[] = {"READHEAD", "SET", "QUIT", "ANSOK",
				"ERR", "ADD", "READCRC", 
				"TAB", "READCRC_TAB", "READROW", 
				"LOG", "SAVELOG", 
				"RLO", "SENDLOG", "SENDNUL", "ASK"};

static void(*statusTable[])(struct paramThr *) = {
	coreReadHead,
	coreSet,
	coreQuit,
	coreOk,
	coreErr,
	coreAdd,
	coreReadCRC,
	coreTab,
	coreReadCRC_Tab,
	coreReadRow,
	coreLog,
	coreSaveLog,
	coreReturnLog,
	coreSendLog,
	coreSendNul,
	coreAsk
};

/* ----------------------------------------------------------------------------
 * @brief	разбирает вход сообщения SET - изменения для табл роутов
 *				      LOG - лог команд для устр 485
 *				      TAB - таблица роутов
 *				      RLO - запрос лога    
 * @return	[] тип пришедшего сообщения SET/TAB[1]
 *					    LOG[2]
 *					    ERR[-1]
 *					    ANS [3]
 */
int bo_master_core(struct paramThr *p)
{
	int typeMSG = -1;
	int stop = 1;
	p->status = READHEAD;
	
	while(stop) {
		/* dbgout("KA[%s]\n", coreStatusTxt[p->status]); */ 
		if(p->status == SET) typeMSG = 1; 
		if(p->status == TAB) typeMSG = 1;
		if(p->status == LOG) typeMSG = 2;
		if(p->status == ERR) typeMSG = -1; 
		if(p->status == ANS) typeMSG = 3;
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
	/* если пришел LOG то в сокете надо оставить заголовок для коррект
	   работы bo_master_core_log */
	exec = recv(p->sock, buf, 3, MSG_PEEK);
	if(exec == -1) {
		bo_log("bo_net_master_core.c coreReadHead() errno[%s]", 
			strerror(errno));
		p->status = ERR;
	} else {
		if(strstr(buf, "LOG")) p->status = LOG;
		else {
			exec = bo_recvAllData(p->sock, (unsigned char *)buf, 3, 3);
			if(exec == -1) {
				bo_log("bo_net_master_core.c coreReadHead() errno[%s]", 
					strerror(errno));
				p->status = ERR;
			} else {
				if(strstr(buf, "SET")) p->status = SET;
				else if(strstr(buf, "TAB")) p->status = TAB;
				else if(strstr(buf, "RLO")) p->status = RLO;
				else if(strstr(buf, "OK") )  p->status = QUIT;
				else if(strstr(buf, "ASK")) p->status = ANS;
				else p->status = ERR;
			}
		}
	}	
}

/* ----------------------------------------------------------------------------
 * @brief	обработка SET|LEN|DATA прием таблицы построчно
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
	if(checkCRC(p) == -1) p->status = ERR;
	else p->status = ADD;
}

static void coreAdd(struct paramThr *p) 
{
	char addr485[4] = {0};
	char value[18] = {0};
	int exec = 0;
	memcpy(addr485, p->buf, 3);
	/* 4 = 1':' + addr(3)*/
	memcpy(value, (p->buf + 4), p->length - 4);
	
	/* value должен быть строкой обяз-но!!! тк функция ht_put() принимает 
	 * в качестве параметра строку*/
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

/* ----------------------------------------------------------------------------
 * @brief	polling сервера. Проверка сокета
 */
static void coreAsk(struct paramThr *p) 
{ 
	int exec = -1;
	unsigned char msg[] = "ASK";
	exec = bo_sendAllData(p->sock, msg, 3);
	if(exec == -1) bo_log("coreAns() errno[%s]", strerror(errno));
	p->status = QUIT;
}
/* ----------------------------------------------------------------------------
 * @brief       проверка CRC
 * @return	[1] - OK [-1] ERR  
 */
STATIC int checkCRC(struct paramThr *p) 
{
	int ans = -1;
	int crc = -1;
	int count = -1;
	unsigned char crcTxt[2] = {0};
	char *msg = (char *)p->buf;
	int msg_len = p->length - 2;
	crcTxt[0] = p->buf[p->length - 2];
	crcTxt[1] = p->buf[p->length - 1];
	
	crc = boCharToInt(crcTxt);
	count = crc16modbus(msg, msg_len);
	p->length = msg_len;
	if(crc != count) ans = -1;
	else ans = 1;
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	создание сообщения ROW|LEN|DATA|...ROW|LEN|DATA|CRC из таблицы tr
 * @buf		буфер куда будет записана таблица
 * @return	[0]  - таблица пустая, пакет не сформ
 *		[>0] - строка создана, размер пакета
 *		[-1] - ошибка, все данные не помещ в буфер 
 */
int bo_master_crtPacket(TOHT *tr, char *buf)
{
	int ans = 0;
	int i;
	int ptr = 0; int data_len;
	char *key, *val;
	char row[50] = {0};
	unsigned char lenStr[2] = {0};
	
	for(i = 0; i < tr->size; i++) {
		key = *(tr->key + i);
		if(key != NULL) {
			/* check buf overflow */
			if(ptr >= BO_MAX_TAB_BUF) {
				ans = -1; 
				goto end;
			}
			/* create |DATA| = {KEY:VAL} */
			val = *(tr->val + i);
			data_len = 0;
			memset(row, 0, 50);
			memcpy(row, key, 3);
			row[3] = ':';
			memcpy(row + 4, val, strlen(val));
			data_len = 4 + strlen(val);
			
			/*ROW(3byte)+LEN(2byte)*/
			boIntToChar(data_len, lenStr);
			memcpy(buf + ptr, "ROW", 3);
			ptr +=3;
			memcpy(buf + ptr, lenStr, 2);
			ptr +=2;
			memcpy(buf + ptr, row, data_len);
			ptr += data_len;
		}
	}
	
	ans = ptr;
	end:
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	отправка таблицы одним пакетом. Один раз нужно вызвать 
 *		gen_tbl_crc16modbus() для создания CRC
 * @return	[-1] - ошибка
 *		[1]  - успешно
 */
int bo_master_sendTab(int sock, TOHT *tr, char *buf)
{
	int ans = -1;
	int len = -1;
	int crc = 0;
	unsigned char crcTxt[2] = {0};
	len = bo_master_crtPacket(tr, buf);
	if(len == -1) {
		bo_log("bo_master_sendTab() error can't set all value in buffer");
	} else if(len > 0) {
		crc = crc16modbus(buf, len);
		boIntToChar(crc, crcTxt);
		len +=2;
		*(buf + len - 2) = crcTxt[0];
		*(buf + len - 1) = crcTxt[1];
		ans = bo_sendTabMsg(sock, buf, len);
		if(ans == -1) {
			bo_log("bo_master_sendTab() error when send Tab");
		}
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	обработка TAB|LEN|DATA прием таблицы построчно
 *			  LEN - 2 byte
 *			  DATA - ROW|LEN|DATA
 */
static void coreTab(struct paramThr *p)
{
	int length = 0;
	int flag  = -1;
	int count = 0;
	length = bo_readPacketLength(p->sock);
	/* длина сообщения минимум 11 байта = 9(ROW|LEN(2)|NULL) байт информации + 2 байта CRC */
	memset(p->buf, 0, p->bufSize);
	if((length > 10) & (length <= p->bufSize)) {
		count = bo_recvAllData(p->sock, 
				       p->buf,
			               p->bufSize,
				       length);
		if((count > 0) & (count == length)) flag = 1; 
		else {
			bo_log("bo_net_master_core.c coreTab() count[%d]!=length[%d]", count, length);
			bo_log("bo_net_master_core.c coreTab() buf[%s]", p->buf);
		}
	} else {
		bo_log("bo_net_master_core.c coreTab() bad length[%d] ", 
			length, 
			p->bufSize);
	}
	if(flag == 1) {
		p->length = length;
		p->status = READCRC_TAB;
	} else {
		p->status = ERR;
	}
}

static void coreReadCRC_Tab(struct paramThr *p)
{
	if(checkCRC(p) == -1) {
		p->status = ERR;
		bo_log("coreReadCRC_Tab() bad CRC");
	} else p->status = READROW;
}

STATIC void coreReadRow(struct paramThr *p)
{
	unsigned char *ptr;
	char head[4] = {0};
	unsigned char lenStr[2];
	char row[18] = {0}; /* XXX.XXX.XXX.XXX:X*/
	char addr485[4] = {0};
	int len;
	int i = 0, error = 1;
	
	ptr = p->buf;
	while(i < p->length) {
		memset(head, 0, 4);
		memset(row, 0, 18);
		memset(addr485, 0, 3);
		
		memcpy(head, ptr, 3);
		ptr += 3; i +=3;
		if(!strstr(head, "ROW")) goto error;
		memcpy(lenStr, ptr, 2);
		ptr += 2; 
		i += 2;
		len = boCharToInt(lenStr);
		if(len > 0) {
			if(i >= p->length) goto error;
			i+= len;
			memcpy(addr485, ptr, 3);
			ptr += 4; /* XXX + ':' (3 + 1) */
			memcpy(row, ptr, len-4);
			ptr += len-4;
			ht_put(p->route_tab, addr485, row);
		} else goto error;
	}
	
	p->status = ANSOK;
	if(error == -1) {
error:
		bo_log("coreReadRow() bad tab format recv");
		p->status = ERR;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	обработка LOG|LEN|DATA|CRC - принимаем лог устр 485
 *			  
 */
static void coreLog(struct paramThr *p)
{
	int length = 0;
	length = bo_master_core_log(p->sock, (char *)p->buf, p->bufSize);
	if(length == -1) {
		p->status = ERR;
		bo_log("coreLog() can't read Log");
	} else {
		p->length = length;
		p->status = SAVELOG;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	сохраняем Лог в цикл связ список  
 */
static void coreSaveLog(struct paramThr *p)
{
	int i = 0;
	int exec = -1;

	dbgout("\nlen[%d]\ncoreSaveLog = [", p->length);
	for(; i < p->length; i++) {
		dbgout( "%c", *(p->buf + i) );
	}
	dbgout("]\n");
	
	exec = bo_cycle_arr_add(p->log, p->buf, p->length);
	if(exec == -1) {
		p->status = ERR;
		bo_log("coreSaveLog() ERROR bo_cycle_arr_add() log");
	} else p->status  = QUIT;
}

static void coreReturnLog(struct paramThr *p) 
{
	int index = -1;
	int exec = -1;
	
	index = bo_readPacketLength(p->sock);
	if(index == -1) {
		p->status = ERR;
		bo_log("coreReturnLog() RLO length[%d]", index);
	} else {
		exec = bo_cycle_arr_get(p->log, p->buf, index);
		p->length = exec;
		if(exec == 0) {
			p->status = SENDNUL;
		} else if(exec == -1) {
			p->status = ERR;
			bo_log("coreReturnLog()->bo_cycle_arr_get() return -1");
		} else {
			p->status = SENDLOG;
		}
	}
}

static void coreSendLog(struct paramThr *p)
{
	unsigned int crc = 0;
	unsigned char crcTxt[2] = {0};
	int ptr = p->length;
	int exec = -1;
	
	crc = crc16modbus((char *)p->buf, ptr);
	boIntToChar(crc, crcTxt);
	memcpy( (p->buf+ptr), crcTxt, 2);
	p->length += 2;
	
	exec = bo_sendLogMsg(p->sock, (char *)p->buf, p->length);
	if(exec == -1) { 
		p->status = ERR;
		bo_log("coreSendLog() can't send log");
	} else {
		/* wait recv OK */
		p->status = QUIT;
	}
}

static void coreSendNul(struct paramThr *p)
{
	int exec = -1;
	unsigned char msg[] = "NUL";
	exec = bo_sendAllData(p->sock, msg, 3);
	if(exec == -1) bo_log("coreSendNul() errno[%s]", strerror(errno));
	p->status = QUIT;
}

/* 0x42 */