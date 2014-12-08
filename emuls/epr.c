
#include "pr.h"
#include "ocfg.h"
#include "ocrc.h"
#include "bologging.h"
#include "serial.h"


int main(int argc, char *argv[])
{
	pthread_t thread_actx;

	struct actx_thread_arg actx_targ;
	
	int result;

	char cfile[64];
	char lfile[64];
	char lfile_old[64];

	int rs_parity, rs_databits, rs_stopbit;
	int rs_speed;
	int rs_port;	
	int tout;

	TOHT *cfg;
	
	if( argc == 2)
		sprintf(cfile, argv[1]);
	else {
		sprintf(cfile, "pr_default.cfg");
		gen_pr_default_cfg(cfile);  /** Генерация конфиг файла
					     * по умолчанию */
	}
	
	cfg = cfg_load(cfile);  /** Загрузили общий конфиг */

	
	/** Установка лог файла */
	sprintf(lfile, cfg_getstring(cfg, "log:log", NULL));
	sprintf(lfile_old, cfg_getstring(cfg, "log:log_old", NULL));
	bo_setLogParam(lfile, lfile_old, 0, 1000);

	/** Генерация таблицы для расчета контрольной суммы
	 * кадра сети RS485 по алгоритму CRC16-MODBUS */
	gen_tbl_crc16modbus();
	
	/** Загрузка параметров серийного порта */
	rs_port = cfg_getint(cfg, "RS:port", -1) - 1;
	/** 0: none, 1: odd, 2: even, 3: space, 4: mark */
	rs_parity = cfg_getint(cfg, "RS:prmParity", -1);
	/** 5 .. 8 */
	rs_databits = cfg_getint(cfg, "RS:prmDatabits", -1);
	/** 1, 2 */
	rs_stopbit = cfg_getint(cfg, "RS:prmStopbit", -1);
	/** Скорость канала RS485 */
	rs_speed = cfg_getint(cfg, "RS:speed", -1);

	tout = cfg_getint(cfg, "RS:tout", -1);

	actx_targ.adr1 = cfg_getint(cfg, "PR1:adr", -1);
	actx_targ.adr2 = cfg_getint(cfg, "PR2:adr", -1);
	
	actx_targ.uso1 = cfg_getint(cfg, "USO:adr1", -1);
	actx_targ.uso2 = cfg_getint(cfg, "USO:adr2", -1);
	actx_targ.uso3 = cfg_getint(cfg, "USO:adr3", -1);
	actx_targ.uso4 = cfg_getint(cfg, "USO:adr4", -1);
	
	/** Установка параметров и открытие серийного порта */
	SerialSetParam(rs_port, rs_parity, rs_databits, rs_stopbit);
	SerialOpen(rs_port);
	SerialSetSpeed(rs_port, rs_speed);
	printf("Open port%d, speed= %d\n", rs_port+1, rs_speed);

	cfg_free(cfg);
	
	bo_log("Init ok");
	printf("Init ok\n");

	
	pthread_attr_init(&pattr);
	pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);

	pthread_mutex_init(&mx_actx, NULL);
	
	init_thrRxBuf(&rxBuf);
	init_thrTxBuf(&txBuf);


	actx_targ.port = rs_port;
	actx_targ.tout = tout;
	result = pthread_create(&thread_actx, &pattr, &actx_485, &actx_targ);
	if (result) {
		printf ("thread_actx: result = %d: %s\n", result,
			strerror(errno));
		return 1;
	}


	/** Ожидаем завершения потоков */
	if (!pthread_equal(pthread_self(), thread_actx))
		pthread_join(thread_actx, NULL);

	
	/** Разрушаем блокировки и условные переменные, освобождая память. */
	
	pthread_mutex_destroy(&mx_actx);
	
	destroy_thrRxBuf(&rxBuf);
	destroy_thrTxBuf(&txBuf);
	
	pthread_exit(0);
}

