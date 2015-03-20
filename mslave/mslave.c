
#if defined (__MOXA_TARGET__) && defined (__WDT__)
#include "mxwdg.h"
#endif

#include "slave.h"
#include "ocfg.h"
#include "ort.h"
#include "ocrc.h"
#include "ofnv1a.h"
#include "bologging.h"
#include "serial.h"
#include "bo_fifo_out.h"
#include "bo_net_master_core.h"


int main(int argc, char *argv[])
{
	pthread_t thread_wdt;
#ifdef __LOG__
	pthread_t thread_log;
#endif
#ifdef __SNMP__
	pthread_t thread_snmpSrv;
#endif
	pthread_t thread_fifoSrv;
	pthread_t thread_sendfifo;
	pthread_t thread_ch1, thread_ch2;
	pthread_t thread_rtSend, thread_rtRecv;
	
	struct wdt_thread_arg wdt_targ;
#ifdef __LOG__
	struct log_thread_arg log_targ;
#endif
#ifdef __SNMP__
	struct snmp_thread_arg snmpSrv_targ;
#endif
	struct fifo_thread_arg fifoSrv_targ;
	struct sfifo_thread_arg sendfifo_targ;
	struct chan_thread_arg ch1_targ, ch2_targ;
	struct rt_thread_arg rtSend_targ, rtRecv_targ;

	int result;
	
	char cfile[64];
	char lfile[64];
	char lfile_old[64];
	char lifeFile[64];

	int rs_parity, rs_databits, rs_stopbit;
	int rs_speed;

	/** unsigned int utxdel; */
	
	int tscan;
	int tout, tout_scan, nretries;

	int wdt_en;
	unsigned long wdt_tm;
	
	int ch1_enable, ch2_enable;
	
	char ip[16];
	unsigned int fifo_port;

	char rt_ipSend[16];
	char rt_ipRecv[16];
	unsigned int rt_portSend, rt_portRecv;

#ifdef __LOG__
	char logSend_ip[16];
	unsigned int logSend_port, logMaxLines;
#endif
	
#ifdef __SNMP__
	int snmp_n, snmp_uso;
	char *tempip;
#endif
	
	char ls_gen[32];
	char req_ag[32];
	char req_ns[32];
	char req_gl[32];
	char req_ms[32];

	int cdDest;
	int cdaId;
	int cdnsId;
	int cdquLogId;
	int cdmsId;
	
	int i;
	char key[7] = {0};
	
	TOHT *cfg;

	if( argc == 2)
		sprintf(cfile, argv[1]);
	else {
		sprintf(cfile, "mslave_default.cfg");
		gen_moxa_default_cfg(cfile);  /** Генерация slave конфиг файла
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
	tscan = cfg_getint(cfg, "moxa:Tscan", -1);
	tout = cfg_getint(cfg, "moxa:Tout", -1);
	tout_scan = cfg_getint(cfg, "moxa:Tout_scan", -1);
	nretries = cfg_getint(cfg, "moxa:nRetries", -1);
	
	/** Установка параметров WatchDog */
	wdt_targ.tsec = cfg_getint(cfg, "WDT:Tsec", -1);
	wdt_targ.tusec = cfg_getint(cfg, "WDT:Tusec", -1);
	wdt_tm = (unsigned long)cfg_getint(cfg, "WDT:Tms", -1);
	wdt_en = cfg_getint(cfg, "WDT:enable", -1);
	/** Инициализация файла для контроля жизни программы через CRON */
	sprintf(lifeFile, cfg_getstring(cfg, "WDT:lifeFile", NULL));
	/* gen_moxa_cron_life(lifeFile); */

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

#ifdef __LOG__
	/** LOGGER server IP, port, максимальное количество строк */
	sprintf(logSend_ip, cfg_getstring(cfg, "LOGGER:sendIp", NULL));
	logSend_port = (unsigned int)cfg_getint(cfg, "LOGGER:sendPort", -1);
	logMaxLines = (unsigned int)cfg_getint(cfg, "LOGGER:maxLines", -1);
#endif
	
#ifdef __SNMP__
	/** SNMP */
	snmp_n = cfg_getint(cfg, "SNMP:n", -1);
	/** Адрес УСО */
	snmp_uso = cfg_getint(cfg, "SNMP:uso", -1);
	if (snmp_n > 0 && snmp_n <= SNMP_IP_MAX)
		for (i=0; i<snmp_n; i++) {
			sprintf(key, "SNMP:%01d", i+1);
			snmpSrv_targ.ip[i] = malloc(sizeof(char) * 16);
			tempip = cfg_getstring(cfg, key, NULL);
			memcpy(*(snmpSrv_targ.ip+i), tempip, strlen(tempip)+1);
		}
#endif
	
	/** Загрузка параметров серийного порта */
	/** 0: none, 1: odd, 2: even, 3: space, 4: mark */
	rs_parity = cfg_getint(cfg, "RS:prmParity", -1);
	/** 5 .. 8 */
	rs_databits = cfg_getint(cfg, "RS:prmDatabits", -1);
	/** 1, 2 */
	rs_stopbit = cfg_getint(cfg, "RS:prmStopbit", -1);
	/** Скорость канала RS485 */
	rs_speed = cfg_getint(cfg, "RS:speed", -1);
	
	/** Длительность одного бита (в микросекундах)
	    вычисляется по формуле: T= 1000000 / V, где V -
	    скорость передачи в бодах. Например, для
	    скорости 19200 бод длительность одного
	    бита составляет 1000000/19200= 52 мкс.
	*/
	/** коэффициент для задержки на время передачи по серийному каналу
	utxdel = 1000000 / rs_speed * 10; */
	
	/** Главная подсистема ЛС 'General' */
	sprintf(ls_gen, cfg_getstring(cfg, "LS:gen", NULL));
	cdDest = mfnv1a(ls_gen);
	
	/** Разрешение на выдачу данных 'AccessGranted' */
	sprintf(req_ag, cfg_getstring(cfg, "REQ:ag", NULL));
	cdaId = mfnv1a(req_ag);
	
	/** Состояние сети RS485 */
	sprintf(req_ns, cfg_getstring(cfg, "REQ:ns", NULL));
	cdnsId = mfnv1a(req_ns);
	
#ifdef __LOG__
	/** Запрос лога */
	sprintf(req_gl, cfg_getstring(cfg, "REQ:gl", NULL));
	cdquLogId = mfnv1a(req_gl);
#endif
	
#ifdef __SNMP__
	/** Состояние магистрали */
	sprintf(req_ms, cfg_getstring(cfg, "REQ:ms", NULL));
	cdmsId = mfnv1a(req_ms);
#endif
	
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


	/** Выделение памяти под буфер для глобальной таблицы
	 * маршрутов загружаемой с контроллера master */
	rtBuf = (unsigned char *)malloc(BO_MAX_TAB_BUF);
	if(rtBuf == NULL) {
		bo_log("main() ERROR %s", "can't alloc mem for rtBuf");
		return 1;
	}

	bo_log("Init ok");
	printf("Init ok\n");

	
	if (bo_init_fifo_out(10) == -1) {	
		bo_log("bo_init_fifo_out() ERROR");
		return 1;
	}
	
	
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
	pthread_mutex_init(&mx_actFIFO, NULL);
	pthread_mutex_init(&mx_rtl, NULL);
	pthread_mutex_init(&mx_rtg, NULL);

#ifdef __LOG__
	pthread_mutex_init(&mx_sendSocket, NULL);
#endif

	init_thrState(&psvdata_ready);
	init_thrState(&psvAnsdata_ready);
	init_thrState(&actFIFOdata_ready);

	init_thrRxBuf(&rxBuf);
	init_thrTxBuf(&txBuf);
	init_thrDstBuf(&dstBuf);
	
	init_thrRxBuf(&rx2Buf);
	init_thrTxBuf(&tx2Buf);
	init_thrDstBuf(&dst2Buf);

	/** Инициализация условных переменных с выделением памяти */
	pthread_cond_init(&psvdata, NULL);
	pthread_cond_init(&psvAnsdata, NULL);
	pthread_cond_init(&actFIFOdata, NULL);


	/** Таблицы маршрутов */
	rtl = ht_new(0);  /** собственный узел */	
	rtg = ht_new(0);  /** внешние узлы */
	
	rtSend_sock = 0;
	rtRecv_sock = 0;

	fifo_idx = 0;

#ifdef __LOG__
	logSend_sock = 0;

	log_targ.logSend_ip = logSend_ip;
	log_targ.logSend_port = logSend_port;
	result = pthread_create(&thread_log, &pattr, &logSendSock_connect, &log_targ);
	if (result) {
		printf("th_log: result = %d: %s\n", result, strerror(errno));
		return 1;
		}
#endif

	rtRecv_targ.ip = rt_ipRecv;
	rtRecv_targ.port = rt_portRecv;
	rtRecv_targ.host_ip = ip;
	result = pthread_create(&thread_rtRecv, &pattr, &rtbl_recv, &rtRecv_targ);
	if (result) {
		printf("th_rtRecv: result = %d: %s\n", result, strerror(errno));
		return 1;
	}

	rtSend_targ.ip = rt_ipSend;
	rtSend_targ.port = rt_portSend;
	rtSend_targ.host_ip = ip;
	result = pthread_create(&thread_rtSend, &pattr, &rtbl_send, &rtSend_targ);
	if (result) {
		printf("th_rtSend: result = %d: %s\n", result, strerror(errno));
		return 1;
	}

#ifdef __SNMP__
	if (snmp_n > 0 && snmp_n <= SNMP_IP_MAX) {
		snmpSrv_targ.n = snmp_n;
	
		printf("ip=%s, n=%d\n", (snmpSrv_targ.ip[0]), snmpSrv_targ.n);
	
		result = pthread_create(&thread_snmpSrv, &pattr, &snmp_serv, &snmpSrv_targ);
		if (result) {
			printf("th_snmpSrv: result = %d: %s\n", result, strerror(errno));
			return 1;
		}

		printf("snmp start ok\n");
	}
#endif
	
	fifoSrv_targ.port = fifo_port;
	result = pthread_create(&thread_fifoSrv, &pattr, &fifo_serv, &fifoSrv_targ);
	if (result) {
		printf("th_fifoSrv: result = %d: %s\n", result, strerror(errno));
		return 1;
		}

	write(1, "fifo start ok\n", 14);
		
	sendfifo_targ.port = fifo_port;
	result = pthread_create(&thread_sendfifo, &pattr, &send_fifo, &sendfifo_targ);
	if (result) {
		printf("th_sendfifo: result = %d: %s\n", result, strerror(errno));
		return 1;
		}

	write(1, "send_fifo start ok\n", 14);
	
	ch1_targ.ch1_enable = ch1_enable;
	ch1_targ.ch2_enable = ch2_enable;
	ch1_targ.tscan = tscan;
	ch1_targ.tout = tout;
	ch1_targ.tout_scan = tout_scan;
	/** ch1_targ.utxdel = utxdel; */
	ch1_targ.wdt_en = wdt_en;
	ch1_targ.nretries = nretries;
	ch1_targ.ip = ip;
	ch1_targ.fifo_port = fifo_port;
#ifdef __SNMP__
	ch1_targ.snmp_n = snmp_n;
	ch1_targ.snmp_uso = snmp_uso;
	ch1_targ.cdmsId = cdmsId;
#endif
#ifdef __LOG__
	ch1_targ.logMaxLines = logMaxLines;
	ch1_targ.cdquLogId = cdquLogId;
#endif
	ch1_targ.cdDest = cdDest;
	ch1_targ.cdaId = cdaId;
	ch1_targ.cdnsId = cdnsId;
	result = pthread_create(&thread_ch1, &pattr, &chan1, &ch1_targ);
	if (result) {
		printf("th_ch1: result = %d: %s\n", result, strerror(errno));
		return 1;
	}

	write(1, "ch1 start ok\n", 13);
	
	ch2_targ.ch1_enable = ch1_enable;
	ch2_targ.ch2_enable = ch2_enable;
	ch2_targ.tscan = tscan;
	ch2_targ.tout = tout;
	ch2_targ.tout_scan = tout_scan;
	/** ch2_targ.utxdel = utxdel; */
	ch2_targ.wdt_en = wdt_en;
	ch2_targ.nretries = nretries;
	ch2_targ.ip = ip;
	ch2_targ.fifo_port = fifo_port;
#ifdef __SNMP__
	ch2_targ.snmp_n = snmp_n;
	ch2_targ.snmp_uso = snmp_uso;
	ch2_targ.cdmsId = cdmsId;
#endif
#ifdef __LOG__
	ch2_targ.logMaxLines = logMaxLines;
	ch2_targ.cdquLogId = cdquLogId;
#endif
	ch2_targ.cdDest = cdDest;
	ch2_targ.cdaId = cdaId;
	ch2_targ.cdnsId = cdnsId;
	result = pthread_create(&thread_ch2, &pattr, &chan2, &ch2_targ);
	if (result) {
		printf("th_ch2: result = %d: %s\n", result, strerror(errno));
		return 1;
	}

	write(1, "ch2 start ok\n", 13);
	
#if defined (__MOXA_TARGET__) && defined (__WDT__)
	if (wdt_en) {

		/* set watch dog timer, must be refreshed in 5ms..60s */
		wdt_fd = mxwdg_open(wdt_tm);
		if (wdt_fd < 0)
		{
			printf("fail to open the watch dog !: %d [%s]\n",
			       wdt_fd, strerror(errno));
			return 1;
		}

		init_thrWdtlife(&wdt_life);
		printf("WatchDog enabled ok\n");
	}
#endif

	wdt_targ.wdt_en = wdt_en;
	wdt_targ.lifeFile = lifeFile;
	result = pthread_create(&thread_wdt, &pattr, &wdt, &wdt_targ);
	if (result) {
		printf("th_wdt: result = %d: %s\n", result, strerror(errno));
		return 1;
		}
	
	
	/** Ожидаем завершения потоков */

#ifdef __LOG__
	if (!pthread_equal(pthread_self(), thread_log))
		pthread_join(thread_log, NULL);
#endif
	
	if (!pthread_equal(pthread_self(), thread_rtRecv))
		pthread_join(thread_rtRecv, NULL);
	if (!pthread_equal(pthread_self(), thread_rtSend))
		pthread_join(thread_rtSend, NULL);

#ifdef __SNMP__
	if (snmp_n > 0 && snmp_n <= SNMP_IP_MAX)
		if (!pthread_equal(pthread_self(), thread_snmpSrv))
			pthread_join(thread_snmpSrv, NULL);
#endif
	
	if (!pthread_equal(pthread_self(), thread_fifoSrv))
		pthread_join(thread_fifoSrv, NULL);
	if (!pthread_equal(pthread_self(), thread_sendfifo))
		pthread_join(thread_sendfifo, NULL);

	if (!pthread_equal(pthread_self(), thread_ch1))
		pthread_join(thread_ch1, NULL);
	if (!pthread_equal(pthread_self(), thread_ch2))
		pthread_join(thread_ch2, NULL);

	if (!pthread_equal(pthread_self(), thread_wdt))
		pthread_join(thread_wdt, NULL);

	
	/** Разрушаем блокировки и условные переменные, освобождаем память. */

	pthread_mutex_destroy(&mx_psv);
	pthread_mutex_destroy(&mx_actFIFO);
	pthread_mutex_destroy(&mx_rtl);
	pthread_mutex_destroy(&mx_rtg);

#ifdef __LOG__
	pthread_mutex_destroy(&mx_sendSocket);
#endif

	destroy_thrRxBuf(&rxBuf);
	destroy_thrTxBuf(&txBuf);
	destroy_thrDstBuf(&dstBuf);
	
	destroy_thrRxBuf(&rx2Buf);
	destroy_thrTxBuf(&tx2Buf);
	destroy_thrDstBuf(&dst2Buf);

	pthread_cond_destroy(&psvdata);
	pthread_cond_destroy(&psvAnsdata);
	pthread_cond_destroy(&actFIFOdata);
	
	destroy_thrState(&psvdata_ready);
	destroy_thrState(&psvAnsdata_ready);
	destroy_thrState(&actFIFOdata_ready);
	
#if defined (__MOXA_TARGET__) && defined (__WDT__)
	if (wdt_en) {
		destroy_thrWdtlife(&wdt_life);
		mxwdg_close(wdt_fd);
	}
#endif

	if(rtBuf != NULL) free(rtBuf);

	rt_free(rtl);
	rt_free(rtg);
	
	return 0;
}

