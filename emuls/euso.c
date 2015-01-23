
#include "uso.h"
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

	char ls_gen[32];
	char req_ag[32];
	char req_ns[32];
	char req_gl[32];
	char req_ms[32];
	char req_sq[32];

	int cdaId;
	int cdnsId;
	int cdquLogId;
	int cdmsId;
	int cdsqId;

	char test_msg[32];
	char snmp_ip[16];
	
	int rs_parity, rs_databits, rs_stopbit;
	int rs_speed;
	int rs_port;	
	int tout;

	TOHT *cfg;
	
	if( argc == 2)
		sprintf(cfile, argv[1]);
	else {
		sprintf(cfile, "uso_default.cfg");
		gen_uso_default_cfg(cfile);  /** Генерация конфиг файла
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
	
	actx_targ.adr = cfg_getint(cfg, "USO:adr", -1);
	actx_targ.logger = cfg_getint(cfg, "USO:logger", -1);
	actx_targ.snmp_q = cfg_getint(cfg, "USO:snmp", -1);

	actx_targ.lline = cfg_getint(cfg, "LOGGER:line", -1);
	actx_targ.nllines = cfg_getint(cfg, "LOGGER:nlines", -1);

	/** Главная подсистема ЛС 'General' */
	sprintf(ls_gen, cfg_getstring(cfg, "LS:gen", NULL));
	actx_targ.cdDest = mfnv1a(ls_gen);
	
	/** Разрешение на выдачу данных 'AccessGranted' */
	sprintf(req_ag, cfg_getstring(cfg, "REQ:ag", NULL));
	cdaId = mfnv1a(req_ag);
	
	/** Состояние сети RS485 */
	sprintf(req_ns, cfg_getstring(cfg, "REQ:ns", NULL));
	cdnsId = mfnv1a(req_ns);
	
	/** Запрос лога */
	sprintf(req_gl, cfg_getstring(cfg, "REQ:gl", NULL));
	cdquLogId = mfnv1a(req_gl);
	
	/** StartQuench */
	sprintf(req_sq, cfg_getstring(cfg, "REQ:sq", NULL));
	cdsqId = mfnv1a(req_sq);
	
	/** Состояние магистрали */
	sprintf(req_ms, cfg_getstring(cfg, "REQ:ms", NULL));
	cdmsId = mfnv1a(req_ms);

	actx_targ.pr = cfg_getint(cfg, "PR:adr", -1);
	
	/** Длина данных сообщения (начальная) */
	actx_targ.test_ln = cfg_getint(cfg, "TEST:ln", -1);
	/** Количество сообщений */
	actx_targ.test_m = cfg_getint(cfg, "TEST:m", -1);
	/** Количество блоков сообщений, если 0- бесконечно долго */
	actx_targ.test_nm = cfg_getint(cfg, "TEST:nm", -1);
	/** Длина сообщения */
	actx_targ.test_msgln = cfg_getint(cfg, "TEST:msg_ln", -1);
	/** Сообщение */
	sprintf(test_msg, cfg_getstring(cfg, "TEST:msg", NULL));

	/** Тест SNMP */
	sprintf(snmp_ip, cfg_getstring(cfg, "SNMP:ip", NULL));
	
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
	actx_targ.cdaId = cdaId;
	actx_targ.cdnsId = cdnsId;
	actx_targ.cdquLogId = cdquLogId;
	actx_targ.cdmsId = cdmsId;
	actx_targ.cdsqId = cdsqId;
	actx_targ.test_msg = test_msg;
	actx_targ.snmp_ip = snmp_ip;
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

