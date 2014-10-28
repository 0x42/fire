#include <stdlib.h>
#include <string.h>
#include <sys/select.h>

#include "../nettcp/bo_net.h"
#include "../tools/oht.h"
#include "../tools/ocfg.h"

static struct {
	int port;
	int queue_len;
	
} servconf = {0};


static void m_readConfig(TOHT *cfg, int n, char **argv);
static void m_servWork(int sock);

void bo_master_main(int argc, char **argv) 
{
	TOHT *cfg = NULL;
	int sock = 0;
	m_readConfig(cfg, argc, argv);
	bo_log("%s%s", " INFO ", "START master");
	if( (sock = bo_servStart(servconf.port, servconf.queue_len))!= -1) {
		m_servWork(sock);
		bo_closeSocket(sock);
	}
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
		bo_setLogParam(f_log, f_log_old, nrow, maxrow);
		bo_log("%s", " WARNING start with default config");
	}
	
}

/* ----------------------------------------------------------------------------
 * @brief	
 */
static void m_servWork(int sock)
{
	int stop = 1;
	int exec = -1;

	/* максимально возможной номер дескриптора*/
	int maxdesc = FD_SETSIZE;
	/* таймер на события */
	struct timeval tval;
	fd_set r_set, w_set, e_set;
	
	tval.tv_sec = 10;
	tval.tv_usec = 0;
	
	while(stop == 1) {
		FD_ZERO(r_set);
		FD_ZERO(w_set);
		FD_ZERO(e_set);
		FD_SET(sock, r_set);
		exec = select(maxdesc, &r_set, NULL, NULL, &tval);
		if(exec == -1) {
			bo_log("bo_net_master.c->m_servWork() select errno[%s]",
				strerror(errno));
			stop = -1;
		} else if(exec == 0) {
			dbgout("server timer event");
		} else {
			if(FD_ISSET(sock, r_set) == 1) {
				
			}
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
static void addClient(int servSock, fd_set *r_set)
{
	int sock = -1;
	/* провер подкл ли кто-нибудь на серверный сокет */
	if(FD_ISSET(servSock, r_set) == 1) {
		sock = accept(servSock, NULL, NULL);
		if(sock == -1) {
			bo_log("addClient() accept errno[%s]", strerror(errno));
		} else {
			bo_setTimerRcv(sock);
			
		}
	}
}

/* 0x42 */