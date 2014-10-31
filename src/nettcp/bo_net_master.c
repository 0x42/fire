#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <fcntl.h>

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
static void m_servWork(int sock_in, struct bo_llsock *llist_in, TOHT *tr);
static void m_addClient(struct bo_llsock *list, int servSock, fd_set *r_set);
static void m_workClientIn(struct bo_llsock *list_in, fd_set *r_set, TOHT *tr);
static void m_recvClientMsg(int sock, TOHT *tr);
static void m_addSockToRSet(struct bo_llsock *list_in, fd_set *r_set);
static int  m_isClosed(struct bo_llsock *list_in, int sock);
/* ----------------------------------------------------------------------------
 * @brief	старт сервера, чтение конфига, созд списка сокетов(текущ подкл) 
 */
void bo_master_main(int argc, char **argv) 
{
	TOHT *cfg   = NULL;
	int sock_in = 0;
	struct bo_llsock *list_in = NULL;
	TOHT *tab_routes = NULL;
	
	gen_tbl_crc16modbus();
	
	m_readConfig(cfg, argc, argv);
	bo_log("%s%s", " INFO ", "START master");
	
	list_in = bo_crtLLSock(servconf.max_con);
	if(list_in == NULL) {
		bo_log("bo_master_main() ERROR %s",
		"can't create list_in haven't free memory");
		goto end;
	}
	
	tab_routes = ht_new(50);
	if(tab_routes == NULL) {
		bo_log("bo_master_main() ERROR %s",
		"can't create tab_routes haven't free memory");
		goto end;
	}
	
	dbgout("server start on port[%d]\n", servconf.port_in);
	if( (sock_in = bo_servStart(servconf.port_in, servconf.queue_len))!= -1) {
		/*переводим сокет в не блок состояние*/
		fcntl(sock_in, F_SETFL, O_NONBLOCK);
		m_servWork(sock_in, list_in, tab_routes);
		bo_closeSocket(sock_in);
	}

end:
	if(list_in != NULL) bo_del_lsock(list_in);
	
	if(tab_routes != NULL) ht_free(tab_routes);
	
	if(cfg != NULL) {
		cfg_free(cfg);
		cfg = NULL;
	}
	bo_log("%s%s", " INFO ", "END master");
}

static void m_readConfig(TOHT *cfg, int n, char **argv) 
{
	int defP = 8890, defQ = 20;
	char *fileName	= NULL;
	char *f_log	= "master_route.log";
	char *f_log_old = "master_route.log(old)";
	int  nrow   = 0;
	int  max_con = 250;
	int  maxrow = 1000;
	servconf.port_in   = defP;
	servconf.queue_len = defQ;
	servconf.max_con   = max_con;
	if(n == 2) {
		fileName = *(argv + 1);
		cfg = cfg_load(fileName);
		if(cfg != NULL) {
			servconf.port_in   = cfg_getint(cfg, "sock:port_in", defP);
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

/* ----------------------------------------------------------------------------
 * @brief	
 * @llist_in		список для хранения сокетов
 * @tr			таблица роутов(для маршрутизации)
 */
static void m_servWork(int sock_in, struct bo_llsock *llist_in, TOHT *tr)
{
	int stop = 1;
	int exec = -1;
	/* максимально возможной номер дескриптора */
	int maxdesc = FD_SETSIZE;
	/* таймер на события */
	struct timeval tval;
	fd_set r_set, w_set, e_set;
	dbgout("m_servWork start\n");

	while(stop == 1) {
		FD_ZERO(&r_set);
		FD_ZERO(&w_set);
		FD_ZERO(&e_set);
		FD_SET(sock_in, &r_set);
		
		m_addSockToRSet(llist_in, &r_set);
		exec = select(maxdesc, &r_set, NULL, NULL, NULL);
		dbgout("select return [%d]\n", exec);
		if(exec == -1) {
			bo_log("bo_net_master.c->m_servWork() select errno[%s]",
				strerror(errno));
			stop = -1;
		} else if(exec == 0) {
			dbgout("... timer event \n");
			/* делаем опрос подкл устройств */
		} else {
			/* если событие произошло у серверного сокета in 
			 * добавляем в список*/
			dbgout("m_servWork() - >event \n");
			m_addClient(llist_in, sock_in, &r_set);
			dbgout("m_servWork() - >check client \n");
			m_workClientIn(llist_in, &r_set, tr);
		}
	}
	/* Очистить все клиент сокеты*/
}

/* ----------------------------------------------------------------------------
 * @brief	если событие произошло на sock то получаем сокет клиента и 
 *		вносим его в список 
 * @servSock    серверный сокет
 * @r_set       битовая маска возв  select
 */
static void m_addClient(struct bo_llsock *list, int servSock, fd_set *r_set)
{
	
	int sock = -1;
	/* провер подкл ли кто-нибудь на серверный сокет */
	if(FD_ISSET(servSock, r_set) == 1) {
		dbgout("m_addClient-> has connect ...\n");
		sock = accept(servSock, NULL, NULL);
		if(sock == -1) {
			bo_log("addClient() accept errno[%s]", strerror(errno));
		} else {
			/* макс время ожид прихода пакета 
				bo_setTimerRcv2(sock, 5, 0);
			*/
			bo_addll(list, sock);
		}
	} else {
		dbgout("m_addClient-> not serv sock \n");
	}
}

/* ----------------------------------------------------------------------------
 * @brief	проверяем сокеты из списка которые отправляют данные серверу
 * @tr		таблица роутов(для маршрутизации)
 */
static void m_workClientIn(struct bo_llsock *list_in, fd_set *r_set, TOHT *tr)
{
	int i = -1;
	int exec = -1;
	int sock = -1;
	struct bo_sock *val = NULL;
	
	i = bo_get_head(list_in);
	while(i != -1) {
		exec = bo_get_val(list_in, &val, i);
		sock = val->sock;
		if(FD_ISSET(sock, r_set) == 1) {
			/* реализовать в потоках ??? <- 0x42*/
			if(m_isClosed(list_in, sock) == 1) 
				m_recvClientMsg(sock, tr);
		}
		i = exec;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	добавляет сокет из списка в множество 
 */
static void m_addSockToRSet(struct bo_llsock *list_in, fd_set *r_set)
{
	int i = -1;
	int exec = -1;
	int sock = -1;
	struct bo_sock *val = NULL;
	
	i = bo_get_head(list_in);
	while(i != -1) {
		exec = bo_get_val(list_in, &val, i);
		sock = val->sock;
		if(sock != -1) {
			FD_SET(sock, r_set);
		} else {
			bo_log("ERROR bo_net_master.c m_addSockToRSet() %s",
				"list return bad sock descriptor");
		}
		i = exec;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	если сокет закрыт удаляем его из списка
 * @return      [1] сокет не закрыт [-1] сокет закрыт
 */
static int m_isClosed(struct bo_llsock *list_in, int sock)
{
	int fl = -1;
	int ans = 1;
	char buf;
	fl = recv(sock, &buf, 1, MSG_PEEK);
	if(fl < 1) {
	/* сокет закрыт удаляем из списка */
		bo_closeSocket(sock);
		bo_del_bysock(list_in, sock);
		ans = -1;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	читаем данные которые отправил клиент
 * @sock	клиентский сокет
 * @tr		таблица роутов(для маршрутизации)
 */
static void m_recvClientMsg(int sock, TOHT *tr)
{
	struct paramThr p;
	int bufSize = 80;
	unsigned char buf[bufSize];
	
	dbgout("m_recvClientMsg() sock is set[%d] \n", sock);
	p.sock = sock;
	p.route_tab = tr;
	p.buf = buf;
	p.bufSize = bufSize;
	p.length = 0;
	bo_master_core(&p);
}

/* 0x42 */