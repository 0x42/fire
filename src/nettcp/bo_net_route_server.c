#include <stdlib.h>
#include <string.h>
#include "../tools/dbgout.h"
#include "../log/bologging.h"
#include "bo_net.h"
#include "../tools/ocfg.h"

static void readConfig(TOHT *cfg, int n, char **argv);

/* ----------------------------------------------------------------------------
 * @port	- порт на котором слушает сервер
 * @queue_len	- очередь запросов ожид-х обраб сервером
 */
static struct {
	int port;
	int queue_len;
} servconf = {0};

/* ----------------------------------------------------------------------------
 * @brief		Запуск сервера ROUTE(хран глоб. табл адресов 
 *			ip, addr485, port485).
 */
void bo_route_main(int n, char **argv)
{
	int sock = 0;
	TOHT *cfg = NULL;
	readConfig(cfg, n, argv);
	bo_log("%s%s", " INFO ", "START moxa_route");
	
	if( (sock = bo_servStart(servconf.port, servconf.queue_len)) != -1) {
		
	
		if(close(sock) == -1) {
			bo_log("%s%s errno[%s]", " ERROR ",
				"bo_route_main()->close()", strerror(errno));
		}
	}
	
	if(cfg != NULL) cfg_free(cfg);
	bo_log("%s%s", " INFO ", "END   moxa_route");
}

static void readConfig(TOHT *cfg, int n, char **argv)
{
	int defP = 8889, defQ = 20;
	char *fileName	= NULL;
	char *f_log	= "moxa_route.log";
	char *f_log_old = "moxa_route.log(old)";
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
		bo_log("%s", " WARNING start with default config");
	}
}
