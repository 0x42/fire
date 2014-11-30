
#include "mxwdg.h"
#include "slave.h"
#include "ch_threads.h"
#include "ocfg.h"
#include "ort.h"
#include "ocrc.h"
#include "ofnv1a.h"
#include "bologging.h"
#include "serial.h"
#include "bo_net_master_core.h"

int  rtBufLen = BO_MAX_TAB_BUF;


int main(int argc, char *argv[])
{
	pthread_t thread_wdt;
	pthread_t thread_fifoSrv;
	pthread_t thread_ch1, thread_ch2;
	pthread_t thread_rtSend, thread_rtRecv;

	struct wdt_thread_arg wdt_targ;
	struct fifo_thread_arg fifoSrv_targ;
	struct chan_thread_arg ch1_targ, ch2_targ;
	struct rt_thread_arg rtSend_targ, rtRecv_targ;
	
	int result;
	
	char cfile[64];
	char lfile[64];
	char lfile_old[64];

	int rs_parity, rs_databits, rs_stopbit;
	int rs_speed;

	int tsec, tusec, tscan;
	int tout, nretries;

#ifdef MOXA_TARGET
	int wdt_en;
	unsigned long wdt_tm;
#endif
	
	int ch1_enable, ch2_enable;
	
	char ip[16];
	unsigned int fifo_port;

	char rt_ipSend[16];
	char rt_ipRecv[16];
	unsigned int rt_portSend, rt_portRecv;

	char logSend_ip[16];
	unsigned int logSend_port;
	
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
	
	TOHT *cfg;

	if( argc == 2)
		sprintf(cfile, argv[1]);
	else {
		sprintf(cfile, "mslave.cfg");
		gen_moxa_default_cfg(cfile);  /** Генерация moxa конфиг файла
					       *  по умолчанию */
	}
	
	cfg = cfg_load(cfile);  /** Загрузили общий конфиг */
	
	/** Установка лог файла */
	sprintf(lfile, cfg_getstring(cfg, "moxa:log", NULL));
	sprintf(lfile_old, cfg_getstring(cfg, "moxa:log_old", NULL));
	bo_setLogParam(lfile, lfile_old, 0, 1000);

	/** Генерация таблицы для расчета контрольной суммы
	 * кадра сети RS485 по алгоритму CRC16-MODBUS */
	gen_tbl_crc16modbus();
	
	/** Установка таймеров */
	tsec = cfg_getint(cfg, "moxa:Tsec", -1);
	tusec = cfg_getint(cfg, "moxa:Tusec", -1);
	tscan = cfg_getint(cfg, "moxa:Tscan", -1);
	tout = cfg_getint(cfg, "moxa:Tout", -1);
	nretries = cfg_getint(cfg, "moxa:nRetries", -1);
	
#ifdef MOXA_TARGET
	/** Установка параметров WatchDog */
	wdt_targ.tsec = cfg_getint(cfg, "WDT:Tsec", -1);
	wdt_targ.tusec = cfg_getint(cfg, "WDT:Tusec", -1);
	wdt_tm = (unsigned long)cfg_getint(cfg, "WDT:Tms", -1);
	wdt_en = cfg_getint(cfg, "WDT:enable", -1);
#endif

	/** IP адрес узла */
	sprintf(ip, cfg_getstring(cfg, "eth0:ip", NULL));

	/** FIFO сервер */
	fifo_port = (unsigned int)cfg_getint(cfg, "FIFO:port", -1);
	fifoSrv_targ.qu_len = cfg_getint(cfg, "FIFO:queue_len", -1);
	fifoSrv_targ.len = cfg_getint(cfg, "FIFO:len", -1);

	/** RT SEND server IP, port */
	sprintf(rt_ipSend, cfg_getstring(cfg, "RT:sendIp", NULL));
	rt_portSend = (unsigned int)cfg_getint(cfg, "RT:sendPort", -1);
	
	/** RT RECV server IP, port */
	sprintf(rt_ipRecv, cfg_getstring(cfg, "RT:recvIp", NULL));
	rt_portRecv = (unsigned int)cfg_getint(cfg, "RT:recvPort", -1);

	/** LOGGER server IP, port */
	sprintf(logSend_ip, cfg_getstring(cfg, "LOGGER:sendIp", NULL));
	logSend_port = (unsigned int)cfg_getint(cfg, "LOGGER:sendPort", -1);

	/** Загрузка параметров серийного порта */
	/** 0: none, 1: odd, 2: even, 3: space, 4: mark */
	rs_parity = cfg_getint(cfg, "RS:prmParity", -1);
	/** 5 .. 8 */
	rs_databits = cfg_getint(cfg, "RS:prmDatabits", -1);
	/** 1, 2 */
	rs_stopbit = cfg_getint(cfg, "RS:prmStopbit", -1);
	/** Скорость канала RS485 */
	rs_speed = cfg_getint(cfg, "RS:speed", -1);
	
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
	
	/** Установка параметров и открытие порта 1 RS485 */
	ch1_targ.port = cfg_getint(cfg, "RS485_1:port", -1) - 1;
	ch1_targ.src = cfg_getint(cfg, "RS485_1:adr", -1);
	ch1_targ.dst_beg = cfg_getint(cfg, "RS485_1:dstBeg", -1);
	ch1_targ.dst_end = cfg_getint(cfg, "RS485_1:dstEnd", -1);
	ch1_enable = cfg_getint(cfg, "RS485_1:enable", -1);
	
	if (ch1_enable) {
		SerialSetParam(ch1_targ.port, rs_parity, rs_databits, rs_stopbit);
		SerialOpen(ch1_targ.port);
		SerialSetSpeed(ch1_targ.port, rs_speed);
		printf("Open port%d, speed= %d\n", ch1_targ.port+1, rs_speed);
	} else
		printf("Port1 disabled\n");
	
	/** Установка параметров и открытие порта 2 RS485 */
	ch2_targ.port = cfg_getint(cfg, "RS485_2:port", -1) - 1;
	ch2_targ.src = cfg_getint(cfg, "RS485_2:adr", -1);
	ch2_targ.dst_beg = cfg_getint(cfg, "RS485_2:dstBeg", -1);
	ch2_targ.dst_end = cfg_getint(cfg, "RS485_2:dstEnd", -1);
	ch2_enable = cfg_getint(cfg, "RS485_2:enable", -1);

	if (ch2_enable) {
		SerialSetParam(ch2_targ.port, rs_parity, rs_databits, rs_stopbit);
		SerialOpen(ch2_targ.port);
		SerialSetSpeed(ch2_targ.port, rs_speed);
		printf("Open port%d, speed2= %d\n", ch2_targ.port+1, rs_speed);
	} else
		printf("Port2 disabled\n");

	cfg_free(cfg);

	
	rtBuf = (unsigned char *)malloc(rtBufLen);
	if(rtBuf == NULL) {
		bo_log("main() ERROR %s", "can't alloc mem for rtBuf");
		return 1;
	}

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

	pthread_mutex_init(&mx_psv, NULL);
	pthread_mutex_init(&mx_rtl, NULL);
	pthread_mutex_init(&mx_rtg, NULL);

	pthread_mutex_init(&mx_sendSocket, NULL);
	
	init_thrState(&psvdata_ready);

	init_thrRxBuf(&rxBuf);
	init_thrTxBuf(&txBuf);
	init_thrDstBuf(&dstBuf);
	
	init_thrRxBuf(&rx2Buf);
	init_thrTxBuf(&tx2Buf);
	init_thrDstBuf(&dst2Buf);

	/** Инициализация условных переменных с выделением памяти */
	pthread_cond_init(&psvdata, NULL);


	/** Таблицы маршрутов */
	rtl = ht_new(0);  /** собственный узел */	
	rtg = ht_new(0);  /** внешние узлы */
	
	rtSend_sock = 0;
	rtRecv_sock = 0;

	logSend_sock = 0;
	fifo_idx = 0;
	
#ifdef MOXA_TARGET
	if (wdt_en) {

		/* set watch dog timer, must be refreshed in 5ms..60s */
		wdt_fd = mxwdg_open(wdt_tm);
		if (wdt_fd < 0)
		{
			printf("fail to open the watch dog !: %d [%s]\n",
			       wdt_fd, strerror(errno));
			return 1;
		}

		/**
		wdt_fd = swtd_open();
		if (wdt_fd < 0) {
			printf("Open sWatchDog device fail !: %d [%s]\n",
			       wdt_fd, strerror(errno));
			return 1;
		}
		// enable it and set it 5 seconds /
		swtd_enable(wdt_fd, wdt_t);
		*/

		init_thrWdtlife(&wdt_life);
		printf("WatchDog enabled ok\n");
	}

	wdt_targ.wdt_en = wdt_en;
	result = pthread_create(&thread_wdt, &pattr, &wdt, &wdt_targ);
	if (result) {
		printf("th_wdt: result = %d: %s\n", result, strerror(errno));
		return 1;
		}
#endif
	
	rtRecv_targ.ip = rt_ipRecv;
	rtRecv_targ.port = rt_portRecv;
	result = pthread_create(&thread_rtRecv, &pattr, &rtbl_recv, &rtRecv_targ);
	if (result) {
		printf("th_rtRecv: result = %d: %s\n", result, strerror(errno));
		return 1;
	}

	rtSend_targ.ip = rt_ipSend;
	rtSend_targ.port = rt_portSend;
	result = pthread_create(&thread_rtSend, &pattr, &rtbl_send, &rtSend_targ);
	if (result) {
		printf("th_rtSend: result = %d: %s\n", result, strerror(errno));
		return 1;
	}

	fifoSrv_targ.port = fifo_port;
	result = pthread_create(&thread_fifoSrv, &pattr, &fifo_serv, &fifoSrv_targ);
	if (result) {
		printf("th_fifoSrv: result = %d: %s\n", result, strerror(errno));
		return 1;
		}

	write(1, "fifo start ok\n", 14);
	
	ch1_targ.ch1_enable = ch1_enable;
	ch1_targ.ch2_enable = ch2_enable;
	ch1_targ.tsec = tsec;
	ch1_targ.tusec = tusec;
	ch1_targ.tscan = tscan;
	ch1_targ.tout = tout;
#ifdef MOXA_TARGET
	ch1_targ.wdt_en = wdt_en;
#endif
	ch1_targ.nretries = nretries;
	ch1_targ.ip = ip;
	ch1_targ.fifo_port = fifo_port;
	ch1_targ.logSend_ip = logSend_ip;
	ch1_targ.logSend_port = logSend_port;
	ch1_targ.cdaId = cdaId;
	ch1_targ.cdaDest = cdaDest;
	ch1_targ.cdnsId = cdnsId;
	ch1_targ.cdnsDest = cdnsDest;
	ch1_targ.cdquLogId = cdquLogId;
	ch1_targ.cdquDest = cdquDest;
	result = pthread_create(&thread_ch1, &pattr, &chan1, &ch1_targ);
	if (result) {
		printf("th_ch1: result = %d: %s\n", result, strerror(errno));
		return 1;
	}

	write(1, "ch1 start ok\n", 13);
	
	ch2_targ.ch1_enable = ch1_enable;
	ch2_targ.ch2_enable = ch2_enable;
	ch2_targ.tsec = tsec;
	ch2_targ.tusec = tusec;
	ch2_targ.tscan = tscan;
	ch2_targ.tout = tout;
#ifdef MOXA_TARGET
	ch2_targ.wdt_en = wdt_en;
#endif
	ch2_targ.nretries = nretries;
	ch2_targ.ip = ip;
	ch2_targ.fifo_port = fifo_port;
	ch2_targ.logSend_ip = logSend_ip;
	ch2_targ.logSend_port = logSend_port;
	ch2_targ.cdaId = cdaId;
	ch2_targ.cdaDest = cdaDest;
	ch2_targ.cdnsId = cdnsId;
	ch2_targ.cdnsDest = cdnsDest;
	ch2_targ.cdquLogId = cdquLogId;
	ch2_targ.cdquDest = cdquDest;
	result = pthread_create(&thread_ch2, &pattr, &chan2, &ch2_targ);
	if (result) {
		printf("th_ch2: result = %d: %s\n", result, strerror(errno));
		return 1;
	}

	write(1, "ch2 start ok\n", 13);
	
	
	/** Ожидаем завершения потоков */
#ifdef MOXA_TARGET
	if (!pthread_equal(pthread_self(), thread_wdt))
		pthread_join(thread_wdt, NULL);
#endif

	if (!pthread_equal(pthread_self(), thread_rtRecv))
		pthread_join(thread_rtRecv, NULL);
	if (!pthread_equal(pthread_self(), thread_rtSend))
		pthread_join(thread_rtSend, NULL);

	if (!pthread_equal(pthread_self(), thread_fifoSrv))
		pthread_join(thread_fifoSrv, NULL);

	if (!pthread_equal(pthread_self(), thread_ch1))
		pthread_join(thread_ch1, NULL);
	if (!pthread_equal(pthread_self(), thread_ch2))
		pthread_join(thread_ch2, NULL);

	
	/** Разрушаем блокировки и условные переменные, освобождаем память. */

	pthread_mutex_destroy(&mx_psv);
	pthread_mutex_destroy(&mx_rtl);
	pthread_mutex_destroy(&mx_rtg);

	pthread_mutex_destroy(&mx_sendSocket);
	
	destroy_thrRxBuf(&rxBuf);
	destroy_thrTxBuf(&txBuf);
	destroy_thrDstBuf(&dstBuf);
	
	destroy_thrRxBuf(&rx2Buf);
	destroy_thrTxBuf(&tx2Buf);
	destroy_thrDstBuf(&dst2Buf);

	pthread_cond_destroy(&psvdata);
	destroy_thrState(&psvdata_ready);
	
#ifdef MOXA_TARGET
	if (wdt_en) {
		destroy_thrWdtlife(&wdt_life);
		/** swtd_close(wdt_fd); */
		mxwdg_close(wdt_fd);
	}
#endif

	if(rtBuf != NULL) free(rtBuf);

	rt_free(rtl);
	rt_free(rtg);
	

	return 0;
}

