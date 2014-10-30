#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "../nettcp/bo_net.h"
#include "../tools/oht.h"
#include "../tools/ocfg.h"
#include "../tools/listsock.h"

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
static void m_servWork(int sock);
static void m_addClient(int servSock, fd_set *r_set);

void bo_master_main(int argc, char **argv) 
{
	TOHT *cfg   = NULL;
	int sock_in = 0;
	struct bo_llsock *list_in = NULL;
	
	m_readConfig(cfg, argc, argv);
	bo_log("%s%s", " INFO ", "START master");
	list_in = bo_crtLLSock(servconf.max_con);
	if(list_in == NULL) {
		bo_log("bo_master_main() ERROR %s",
		"can't create list_in haven't free memory");
		goto end;
	}
	
	if( (sock_in = bo_servStart(servconf.port_in, servconf.queue_len))!= -1) {
		m_servWork(sock_in, list_in);
		bo_closeSocket(sock_in);
	}

end:
	if(list_in != NULL) bo_del_lsock(list_in);
	
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
 */
static void m_servWork(int sock_in, struct bo_llsock *llist_in)
{
	int stop = 1;
	int exec = -1;
	int cl_sock = -1;
	/* максимально возможной номер дескриптора */
	int maxdesc = FD_SETSIZE;
	/* таймер на события */
	struct timeval tval;
	fd_set r_set, w_set, e_set;
	
	tval.tv_sec = 10;
	tval.tv_usec = 0;
	
	while(stop == 1) {
		FD_ZERO(&r_set);
		FD_ZERO(&w_set);
		FD_ZERO(&e_set);
		FD_SET(sock_in, &r_set);
		
		exec = select(maxdesc, &r_set, NULL, NULL, &tval);
		if(exec == -1) {
			bo_log("bo_net_master.c->m_servWork() select errno[%s]",
				strerror(errno));
			stop = -1;
		} else if(exec == 0) {
			dbgout("server timer event ");
			/* делаем опрос подкл устройств */
		} else {
			m_addClient(llist_in, sock_in, &r_set);
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
		sock = accept(servSock, NULL, NULL);
		if(sock == -1) {
			bo_log("addClient() accept errno[%s]", strerror(errno));
		} else {
			/* макс время ожид прихода пакета 
				bo_setTimerRcv2(sock, 5, 0);
			*/
			bo_addll(list, sock);
		}
	}
}

/* ----------------------------------------------------------------------------
 * @brief	проверяем сокеты из списка которые отправляют данные серверу
 */
static void m_workClientIn(struct bo_llsock *list_in, fd_set *r_set)
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
			m_recvClientMsg(sock);
		}
		i = exec;
	}
}

/* ----------------------------------------------------------------------------
 * @brief	читаем данные которые отправил клиент
 * @sock	клиентский сокет
 */
static void m_recvClientMsg(int sock)
{
 
}

/* 0x42 */