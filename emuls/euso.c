
#include "uso.h"
#include "uso_threads.h"
#include "ocfg.h"
#include "ocrc.h"
#include "ofnv1a.h"
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
	char cmdfile[64];

	char ls_gen[32];
	char req_ag[32];
	char req_ns[32];
	char req_gl[32];

	int cdaId;
	int cdaDest;
	int cdnsId;
	int cdnsDest;
	int cdquLogId;
	int cdquDest;
	
	int rs_parity, rs_databits, rs_stopbit;
	int rs_speed;
	int rs_port;	
	int tout;

	TOHT *cfg;
	
	if( argc == 2)
		sprintf(cfile, argv[1]);
	else {
		sprintf(cfile, "uso.cfg");
		gen_uso_default_cfg(cfile);  /** Генерация конфиг файла
					      * по умолчанию */
	}
	
	cfg = cfg_load(cfile);  /** Загрузили общий конфиг */

	
	/** Установка лог файла */
	sprintf(lfile, cfg_getstring(cfg, "log:log", NULL));
	sprintf(lfile_old, cfg_getstring(cfg, "log:log_old", NULL));
	bo_setLogParam(lfile, lfile_old, 0, 1000);

	sprintf(cmdfile, cfg_getstring(cfg, "TEST:cmd", NULL));

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
	
	actx_targ.adr1 = cfg_getint(cfg, "USO1:adr", -1);
	actx_targ.logger1 = cfg_getint(cfg, "USO1:logger", -1);

	actx_targ.adr2 = cfg_getint(cfg, "USO2:adr", -1);
	actx_targ.logger2 = cfg_getint(cfg, "USO2:logger", -1);

	actx_targ.logger2 = cfg_getint(cfg, "USO2:logger", -1);

	actx_targ.lline = cfg_getint(cfg, "LOGGER:line", -1);
	actx_targ.nllines = cfg_getint(cfg, "LOGGER:nlines", -1);

	/** Главная подсистема ЛС 'General' */
	sprintf(ls_gen, cfg_getstring(cfg, "LS:gen", NULL));
	
	/** Разрешение на выдачу данных 'AccessGranted' */
	sprintf(req_ag, cfg_getstring(cfg, "REQ:ag", NULL));
	cdaId = mfnv1a(req_ag);
	cdaDest = mfnv1a(ls_gen);
	
	/** Состояние сети RS485 */
	sprintf(req_ns, cfg_getstring(cfg, "REQ:ns", NULL));
	cdnsId = mfnv1a(req_ns);
	cdnsDest = mfnv1a(ls_gen);
	
	/** Запрос лога */
	sprintf(req_gl, cfg_getstring(cfg, "REQ:gl", NULL));
	cdquLogId = mfnv1a(req_gl);
	cdquDest = mfnv1a(ls_gen);
	
	actx_targ.pr1 = cfg_getint(cfg, "PR1:adr", -1);
	actx_targ.pr2 = cfg_getint(cfg, "PR2:adr", -1);
	actx_targ.pr3 = cfg_getint(cfg, "PR3:adr", -1);
	
	/** Установка параметров и открытие серийного порта */
	SerialSetParam(rs_port, rs_parity, rs_databits, rs_stopbit);
	SerialOpen(rs_port);
	SerialSetSpeed(rs_port, rs_speed);
	printf("Open port%d, speed= %d\n", rs_port+1, rs_speed);

	cfg_free(cfg);

	
	cmd = cfg_load(cmdfile);  /** Загрузили тест команды */
	actx_targ.ncmds = cfg_getint(cmd, "CMD:n", -1);
	actx_targ.m = cfg_getint(cmd, "CMD:m", -1);
	
	
	bo_log("Init ok");
	printf("Init ok\n");

	
	/** Установка атрибутов функционирования нитей PTHREAD:
	 * - инициализирует структуру, указываемую pattr, значениями
	     "по умолчанию";
	 * - область действия конкуренции (scope) определяет связность
	 *   потока с LWP;
	 * - отсоединенность (detachstate);
	 * - область действия блокировки PTHREAD_PROCESS_PRIVATE. */
	pthread_attr_init(&pattr);
	pthread_attr_setscope(&pattr, PTHREAD_SCOPE_SYSTEM);
	pthread_attr_setdetachstate(&pattr, PTHREAD_CREATE_JOINABLE);

	pthread_mutex_init(&mx_actx, NULL);
	
	init_thrRxBuf(&rxBuf);
	init_thrTxBuf(&txBuf);


	actx_targ.port = rs_port;
	actx_targ.tout = tout;
	actx_targ.cdaId = cdaId;
	actx_targ.cdaDest = cdaDest;
	actx_targ.cdnsId = cdnsId;
	actx_targ.cdnsDest = cdnsDest;
	actx_targ.cdquLogId = cdquLogId;
	actx_targ.cdquDest = cdquDest;
	result = pthread_create(&thread_actx, &pattr, &actx_485, &actx_targ);
	if (result) {
		printf ("thread_actx: result = %d: %s\n", result,
			strerror(errno));
		return 1;
	}
	

	/** Ожидаем завершения потоков */
	if (!pthread_equal(pthread_self(), thread_actx))
		pthread_join(thread_actx, NULL);

	cfg_free(cmd);
	
	/** Разрушаем блокировки и условные переменные, освобождая память. */
	
	pthread_mutex_destroy(&mx_actx);
	
	destroy_thrRxBuf(&rxBuf);
	destroy_thrTxBuf(&txBuf);
	
	pthread_exit(0);
}

