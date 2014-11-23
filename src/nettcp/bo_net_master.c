#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../nettcp/bo_net.h"
#include "../tools/oht.h"
#include "../tools/ocfg.h"
#include "../tools/listsock.h"
#include "../nettcp/bo_net_master_core.h"

static struct {
	/* на данный порт устройства присылают измен клиенты, лог ПР */
	int port_in;
	/* отправка измен табл роутов, опрос устройств */
	int port_out;
	int queue_len;
	/* макс кол-во утройств могут подкл*/
	int max_con;
} servconf = {0};

static void m_readConfig(TOHT *cfg, int n, char **argv);
static struct bo_llsock *m_crtLL(int size);
static void m_servWork(int sock_in, int sock_out,
	               struct bo_llsock *llist_in,
		       struct bo_llsock *llist_out, TOHT *tr);
static void m_addClient(struct bo_llsock *list, int servSock, fd_set *set);
static void m_addClientOut(struct bo_llsock *list, int servSock, fd_set *set, 
			   TOHT *tr);
static void m_workClient(struct bo_llsock *list_in, struct bo_llsock *list_out,
			   fd_set *r_set, fd_set *w_set, TOHT *tr);
static int  m_recvClientMsg(int sock, TOHT *tr);
static void m_sendClientMsg(int sock, TOHT *tr, struct bo_llsock *llist_out);
static void m_sendTabPacket(int sock, TOHT *tr, struct bo_llsock *list);
static void m_repeatSendRoute(struct bo_llsock *list_out, TOHT *tr);
static void m_delRoute(struct bo_sock *val, TOHT *tr);
static void m_addSockToSet(struct bo_llsock *list_in, fd_set *r_set);
static int  m_isClosed(struct bo_llsock *list_in, int sock);

/* Buffer for recieve data */
static unsigned char *recvBuf;
static int  recvBufLen = BO_MAX_TAB_BUF;
static struct bo_cycle_arr *logArr;
/* ----------------------------------------------------------------------------
 * @brief	старт сервера, чтение конфига, созд списка сокетов(текущ подкл) 
 */
void bo_master_main(int argc, char **argv) 
{
	TOHT *cfg    = NULL;
	int sock_in  = 0;
	int sock_out = 0;
	struct bo_llsock *list_in  = NULL;
	struct bo_llsock *list_out = NULL;
	TOHT *tab_routes = NULL;
	
	gen_tbl_crc16modbus();
	
	m_readConfig(cfg, argc, argv);
	bo_log("%s%s", " INFO ", "START master");
	
	list_in  = m_crtLL(servconf.max_con);
	if(list_in  == NULL) goto end;
	
	list_out = m_crtLL(servconf.max_con);
	if(list_out == NULL) goto end;
	
	recvBuf = (unsigned char *)malloc(recvBufLen);
	if(recvBuf == NULL) {
		bo_log("bo_master_main() ERROR %s",
		"can't alloc mem for recvBuf");
		goto end;
	}
	
	tab_routes = ht_new(50);
	if(tab_routes == NULL) {
		bo_log("bo_master_main() ERROR %s",
		"can't create tab_routes haven't free memory");
		goto end;
	}
	
	logArr = NULL;
	logArr = bo_cycle_arr_init(1024);
	if(logArr == NULL) {
		bo_log("bo_master_main() ERROR %s",
		"can't create logArr haven't free memory");
		goto end;
	}
	
	dbgout("server start on port_in[%d]\n", servconf.port_in);
	sock_in  = bo_servStart(servconf.port_in, servconf.queue_len);
	dbgout("server start on port_out[%d]\n", servconf.port_out);
	sock_out = bo_servStart(servconf.port_out, servconf.queue_len);
	
	if( (sock_in != -1) & (sock_out != -1) ) {
		/* переводим сокет в не блок состояние */
		fcntl(sock_in,  F_SETFL, O_NONBLOCK);
		fcntl(sock_out, F_SETFL, O_NONBLOCK);

		m_servWork(sock_in, sock_out, 
			   list_in, list_out, 
			   tab_routes);
	}
	
	if(sock_in  != -1) bo_closeSocket(sock_in);
	if(sock_out != -1) bo_closeSocket(sock_out);

end:
	if(list_in  != NULL) bo_del_lsock(list_in);
	if(list_out != NULL) bo_del_lsock(list_out);

	if(tab_routes != NULL) ht_free(tab_routes);

	if(recvBuf != NULL) free(recvBuf);

	if(logArr != NULL) bo_cycle_arr_del(logArr);

	if(cfg != NULL) {
		cfg_free(cfg);
		cfg = NULL;
	}
	bo_log("%s%s", " INFO ", "END master");
}

static void m_readConfig(TOHT *cfg, int n, char **argv) 
{
	int defPin = 8890, defPout = 8891, defQ = 20;
	char *fileName	= NULL;
	char *f_log	= "master_route.log";
	char *f_log_old = "master_route.log(old)";
	int  nrow    = 0;
	int  max_con = 250;
	int  maxrow  = 1000;
	servconf.port_in   = defPin;
	servconf.port_out  = defPout;
	servconf.queue_len = defQ;
	servconf.max_con   = max_con;
	if(n == 2) {
		fileName = *(argv + 1);
		cfg = cfg_load(fileName);
		if(cfg != NULL) {
			servconf.port_in   = cfg_getint(cfg, "sock:port_in", defPin);
			servconf.port_out  = cfg_getint(cfg, "sock:port_out", defPout);
			servconf.queue_len = cfg_getint(cfg, "sock:queue_len", defQ);
			servconf.max_con   = cfg_getint(cfg, "sock:max_connect", max_con);
			
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
		bo_setLogParam(f_log, f_log_old, nrow, maxrow);
		bo_log("%s", " WARNING start with default config");
	}
}

static struct bo_llsock *m_crtLL(int size)
{
	struct bo_llsock *ll = NULL;
	ll = bo_crtLLSock(size);
	
	if(ll == NULL) {
		bo_log("m_crtLL() ERROR %s",
		"can't create linked list haven't free memory");
	}
	return ll;
}
/* ----------------------------------------------------------------------------
 * @brief	
 * @llist_in		список сокетов кот отпр измен мастеру
 * @llist_out		список сокетов кот прин измен от мастера
 * @tr			таблица роутов(для маршрутизации)
 */
static void m_servWork(int sock_in, int sock_out,
	               struct bo_llsock *llist_in,
		       struct bo_llsock *llist_out, 
		       TOHT *tr)
{
	int stop = 1;
	int exec = -1;
	/* максимально возможной номер дескриптора для сокета*/
	int maxdesc = FD_SETSIZE;
	fd_set r_set, w_set, e_set;
	/* таймер на события */
	struct timeval tval;

	while(stop == 1) {
		tval.tv_sec  = 0;
		tval.tv_usec = 50;
		FD_ZERO(&r_set);
		FD_ZERO(&w_set);
		FD_ZERO(&e_set);
		FD_SET(sock_in,  &r_set);
		FD_SET(sock_out, &r_set);
		
		m_addSockToSet(llist_in,  &r_set);
		/* если сокет из того списка подаст сигнал на передачу 
		 * значит его надо закрыть.*/
		m_addSockToSet(llist_out, &r_set);
		exec = select(maxdesc, &r_set, NULL, NULL, &tval);
		if(exec == -1) {
			bo_log("bo_net_master.c->m_servWork() select errno[%s]",
				strerror(errno));
			bo_log("CRITICAL ERROR app will stop");
			stop = -1;
		} else if(exec == 0) {
			/* если в списке есть устр которым не удалось отправить
			 * таблицу повторяем отправку */
			m_repeatSendRoute(llist_out, tr);
		} else {
			dbgout("\n------ EVENT ->\n");
			/* если событие произошло у серв сокетов in&out 
			 * добавляем в список*/
			dbgout("CHK SERV IN \n");
			m_addClient(llist_in,  sock_in,  &r_set);
			
			dbgout("CHK SERV OUT \n");
			m_addClientOut(llist_out, sock_out, &r_set, tr);
			/*
			dbgout("IN:    ");   bo_print_list_val(llist_in);
			dbgout("\nOUT:   "); bo_print_list_val(llist_out); 
			*/
			dbgout("\nCHK CLIENT   \n");
			m_workClient(llist_in, llist_out, &r_set, &w_set, tr);
			dbgout("------ END \n");
		}
	}
	/* Очистить все клиент сокеты*/
}

/* ----------------------------------------------------------------------------
 * @brief	если событие произошло на sock то получаем сокет клиента и 
 *		вносим его в список 
 * @servSock    серверный сокет
 * @set       битовая маска возв  select
 */
static void m_addClient(struct bo_llsock *list, int servSock, fd_set *set)
{
	
	int sock = -1;
	/* провер подкл ли кто-нибудь на серверный сокет */
	if(FD_ISSET(servSock, set) == 1) {
		dbgout("m_addClient-> has connect ...\n");
		sock = accept(servSock, NULL, NULL);
		if(sock == -1) {
			bo_log("addClient() accept errno[%s]", strerror(errno));
		} else {
			/* макс время ожид прихода пакета, 
			 * чтобы искл блокировки */
			bo_setTimerRcv2(sock, 5, 500);
			bo_addll(list, sock);
		}
	} else {
		dbgout("m_addClient-> not serv sock \n");
	}
}

/* ----------------------------------------------------------------------------
 * @brief	при подкл клиента отправляем ему таблицу роутов
 */
static void m_addClientOut(struct bo_llsock *list, int servSock, fd_set *set, 
			   TOHT *tr)
{
	int sock = -1;
	/* провер подкл ли кто-нибудь на серверный сокет */
	if(FD_ISSET(servSock, set) == 1) {
		dbgout("m_addClientOut-> has connect ...\n");
		sock = accept(servSock, NULL, NULL);
		if(sock == -1) {
			bo_log("m_addClientOut() accept errno[%s]", strerror(errno));
		} else {
			/* макс время ожид прихода пакета, 
			 * чтобы искл блокировки */
			bo_setTimerRcv2(sock, 5, 500);
			bo_addll(list, sock);
			/* m_sendClientMsg(sock, tr, list); */
			m_sendTabPacket(sock, tr, list);
		}
	} else {
		dbgout("m_addClientOut-> not serv sock \n");
	}
}
/* ----------------------------------------------------------------------------
 * @brief	проверяем сокеты из списка которые отправляют данные серверу
 * @tr		таблица роутов(для маршрутизации)
 */
static void m_workClient(struct bo_llsock *list_in, struct bo_llsock *list_out,
			fd_set *r_set, fd_set *w_set, TOHT *tr)
{
	int i = -1;
	int exec = -1;
	int sock = -1;
	int max_desc = FD_SETSIZE;
	/* флаг получен измен */
	int flag = -1;
	struct bo_sock *val = NULL;
	struct timeval tval;
	tval.tv_sec = 0;
	tval.tv_usec = 50;
	
	
	dbgout("CHK FROM LIST_IN: ");
	i = bo_get_head(list_in);
	while(i != -1) {
		exec = bo_get_val(list_in, &val, i);
		sock = val->sock;
		if(FD_ISSET(sock, r_set) == 1) {
			/* реализовать в потоках ??? <- 0x42 */
			dbgout("sock[%d] set\n", sock);
			if(m_isClosed(list_in, sock) == 1) 
				if(m_recvClientMsg(sock, tr) == 1) flag = 1;
		}
		i = exec;
	}
	/* делаем проверку надо ли закрывать сокеты из списка out
	   удал записи из таблицы роутов для этого сокета */
	dbgout("\nCHK LIST OUT TO CLOSE:  ");
	i = bo_get_head(list_out);
	while(i != -1) {
		exec = bo_get_val(list_out, &val, i);
		sock = val->sock;
		if(FD_ISSET(sock, r_set) == 1) {
			/* реализовать в потоках ??? <- 0x42 */
			dbgout("sock[%d] set \n", sock);
			m_delRoute(val, tr);
			bo_closeSocket(sock);
			bo_del_bysock(list_out, sock);
			flag = 1;
		}
		i = exec;
	}
	/* если было получено SET сообщ делаем рассылку tab route */
	if(flag == 1) {
		exec = -1;
		FD_ZERO(w_set);
		m_addSockToSet(list_out, w_set);
		exec = select(max_desc, NULL, w_set, NULL, &tval);
		if(exec > 0 ) {
			dbgout("\nSEND CHANGE TO:");
			i = bo_get_head(list_out);
			while(i != -1) {
				exec = bo_get_val(list_out, &val, i);
				sock = val->sock;
				if(FD_ISSET(sock, w_set) == 1) {
					dbgout(" sock[%d] ", sock);
					/*m_sendClientMsg(sock, tr, list_out);*/
					m_sendTabPacket(sock, tr, list_out);
				}
				i = exec;
			}
			
			dbgout("\n");
		}
	}
	dbgout("\n");
}

/* ----------------------------------------------------------------------------
 * @brief	добавляет сокет из списка в множество 
 */
static void m_addSockToSet(struct bo_llsock *list, fd_set *set)
{
	int i = -1;
	int exec = -1;
	int sock = -1;
	struct bo_sock *val = NULL;
	
	i = bo_get_head(list);
	while(i != -1) {
		exec = bo_get_val(list, &val, i);
		sock = val->sock;
		if(sock != -1) {
			FD_SET(sock, set);
		} else {
			bo_log("ERROR bo_net_master.c m_addSockToSet() %s",
				"list return bad sock descriptor");
		}
		i = exec;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	если сокет закрыт удаляем его из списка
 * @return      [1] сокет не закрыт [-1] сокет закрыт
 */
static int m_isClosed(struct bo_llsock *list, int sock)
{
	int fl = -1;
	int ans = 1;
	char buf;
	fl = recv(sock, &buf, 1, MSG_PEEK);
	if(fl < 1) {
	/* сокет закрыт удаляем из списка */
		printf("DEL SOCK[%d] m_isClosed\n", sock);
		bo_closeSocket(sock);
		bo_del_bysock(list, sock);
		ans = -1;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	читаем данные которые отправил клиент
 * @sock	клиентский сокет
 * @tr		таблица роутов(для маршрутизации)
 * @return	if recv SET[1], another type of msg recv[0];
 */
static int m_recvClientMsg(int sock, TOHT *tr)
{
	struct paramThr p;
	/*  */
	int t_msg = 0;
	int i; char *key; char *val;

	dbgout("m_recvClientMsg() sock is set[%d] \n", sock);
	p.sock = sock;
	p.route_tab = tr;
	p.buf = recvBuf;
	p.bufSize = recvBufLen;
	p.length = 0;
	p.log = logArr;
	
	t_msg = bo_master_core(&p);

	
	printf("==== TAB ROUTE ==== \n");
	for(i = 0; i < tr->size; i++) {
		key = *(tr->key + i);
		if(key != NULL) {
			val = *(tr->val + i);
			printf("[%s:%s]\n", key, val);
		}
	}
	printf("==== END TAB ==== \n");

	return t_msg;
}

/* ----------------------------------------------------------------------------
 * @DEPRICATED
 * @brief	отправ всю таблицу роутов клиенту(построчно)
 */
static void m_sendClientMsg(int sock, TOHT *tr, struct bo_llsock *list)
{
	int i = 0;
	char *key = NULL;
	char *val = NULL;
	int valSize = 0;
	int crc = 0;
	int exec = 0;
	char ip[BO_IP_MAXLEN] = "null"; /* listsock.h define */
	unsigned char cbuf[2] = {0};
	int packetSize = 23;
	int p_len = 0;
	char packet[packetSize];
	char dbg[packetSize-1];
	dbg[packetSize-2] = '\0';
	/* packet = [XXX:XXX.XXX.XXX.XXX:XXX] / [XXX:NULL];  */
	
	dbgout("отправка табл роутов sock[%d]\n", sock);
	for(i = 0; i < tr->size; i++) {
		key = *(tr->key + i);
		if(key != NULL) {
			memset(packet, 0, packetSize);
			val = *(tr->val + i);
			valSize = strlen(val);
			p_len = valSize;
			if(valSize <= packetSize) {
				/*dbgout("send->val[%s] %d\n", val, valSize);*/
				memcpy(packet, key, 3);
				packet[3] = ':';
				memcpy(packet + 4, val, valSize);
				p_len += 4;
				crc = crc16modbus(packet, p_len);
				
				boIntToChar(crc, cbuf);
				
				packet[p_len] = cbuf[0];
				packet[p_len + 1] = cbuf[1];
				p_len += 2;
				exec = bo_sendSetMsg(sock, packet, p_len);
				if(exec == -1) {
					bo_getip_bysock(list, sock, ip);
					bo_setflag_bysock(list, sock, -1);
					bo_log("m_sendClientMsg() can't send data to ip[%s]", ip);
					memcpy(dbg, packet, p_len);
					bo_log("packet[%s]", dbg);
				} else {
					bo_setflag_bysock(list, sock, 1);
				}
			} else {
				bo_log("m_sendClientMsg() -> ERR valSize[%d]", 
					valSize);
				bo_log("big size val[%s] in table", val);
			}
		}
	}
}

/* ----------------------------------------------------------------------------
 * @brief	отправка таблицы одним пакетом
 */
static void m_sendTabPacket(int sock, TOHT *tr, struct bo_llsock *list)
{
	int exec = 1;
	char ip[BO_IP_MAXLEN] = "null";
	int i = 0;
	char *key = NULL; char *val = NULL;
	int tab_not_empty = -1;
	memset(recvBuf, 0, recvBufLen);
	/* провер-м чтобы таблица не была пустой */
	for(i = 0; i < tr->size; i++) {
		key =  *(tr->key + i);
		if(key != NULL) {
			tab_not_empty = 1;
			break;
		}
	}
	
	if(tab_not_empty == 1) {
		exec = bo_master_sendTab(sock, tr, (char *)recvBuf);
		dbgout("\n==== SEND TAB ====\n");
		for(i = 0; i < tr->size; i++) {
			key = *(tr->key + i);
			if(key != NULL) {
				val = *(tr->val + i);
				dbgout("[%s:%s]\n", key, val);
			}
		}
		dbgout("==== SEND END ====\n");
		if(exec == -1) {
			bo_getip_bysock(list, sock, ip);
			bo_setflag_bysock(list, sock, -1);
			bo_log("m_sendTabPacket() can't send data to ip[%s]", ip);
		} else {
			bo_setflag_bysock(list, sock, 1);
		}
	}
	
}
/* ----------------------------------------------------------------------------
 * @brief 
 */
static void m_repeatSendRoute(struct bo_llsock *list_out, TOHT *tr)
{
	int i    = -1;
	int exec = -1;
	int sock = -1;
	struct bo_sock *val = NULL;
	
	i = bo_get_head(list_out);
	while(i != -1) {
		sock = -1;
		exec = bo_get_val(list_out, &val, i);
		if(val->flag == -1 ) {
			sock = val->sock;
		/*	m_sendClientMsg(sock, tr, list_out); */
			m_sendTabPacket(sock, tr, list_out);
		}
		i = exec;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	наход строки у которых ip = item->ip удал их  
 */
static void m_delRoute(struct bo_sock *item, TOHT *tr)
{
	int i = -1;
	int exec = 0;
	char *key = NULL;
	char *val = NULL;
	char *err_msg = "m_delRoute() ERROR can't change TAB ROUTE ht_put[-1]";
	for(i = 0; i < tr->size; i++) {
		key = *(tr->key + i);
		if(key != NULL) {
			val = *(tr->val + i);
			if(strstr(val, item->ip)) {
				exec = 0;
				exec = ht_put(tr, key, "NULL");
				if(exec == -1) 	bo_log(err_msg);
			}
		}
	}
}

/* 0x42 */