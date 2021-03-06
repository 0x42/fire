#include "bo_snmp_mng.h"

static struct OID_Next *bo_getPortVal(int sock, char *ip, struct PortItem *portItem);
static void bo_checkSwitch(int sock, struct OPT_SWITCH *o_sw);
static int  bo_crt_optSwitch(char *ip[], int n);
static void bo_del_tab_switch();
static void bo_prt_switch(int n);

/* таблица OPTICAL SWITCH */
static struct OPT_SWITCH *tab_sw = NULL;
/* флаг изменения на магистрали 
 * [0] - нет изменений
 * [1] - есть изменения
 */
static int sw_net_chg = 1;

static pthread_mutex_t opt_sw_mut = PTHREAD_MUTEX_INITIALIZER;

void bo_snmp_lock_mut() 
{
	pthread_mutex_lock(&opt_sw_mut);
}

void bo_snmp_unlock_mut() 
{
	pthread_mutex_unlock(&opt_sw_mut);
}

int bo_snmp_isChange()
{
	return sw_net_chg;
}

struct OPT_SWITCH *bo_snmp_get_tab() 
{
	sw_net_chg = 0;
	return tab_sw;
}

void bo_snmp_main(char *ip[], int n)
{
	int stop = 1;
	int exec = -1, sock = -1, i = 0;
	struct OPT_SWITCH *o_sw = NULL;
	
	bo_log("SNMP(monitor) START");
	bo_snmp_lock_mut();
	exec = bo_init_snmp();
	bo_snmp_unlock_mut();
	if(exec == -1) { 
		bo_log("bo_snmp_main ERROR can't create data for run snmp");
		goto exit;
	}
	
	bo_snmp_lock_mut();
	exec = bo_crt_optSwitch(ip, n);
	bo_snmp_unlock_mut();
	if(exec == -1) {
		bo_log("bo_snmp_main ERROR can't create data for OPT_SWITCH");
		goto exit;
	}

	for(i = 0; i < n; i++)  {
		o_sw = tab_sw + i;
		dbgout("bo_snmp_mng INFO SWITCH IP-%d[%s]\n", i, o_sw->ip);
		bo_log("bo_snmp_mng INFO SWITCH IP-%d[%s]", i, o_sw->ip);
	}

	sock = bo_udp_sock();
	if(sock == 0) {
		bo_log("bo_snmp_main ERROR can't create socket for run snmp");
		goto exit;
	}
	
	while(stop == 1) {
		sleep(10);
		dbgout("\n ==================== \n"); 
		for(i = 0; i < n; i++) {
			o_sw = tab_sw + i;
			bo_snmp_lock_mut();
			bo_checkSwitch(sock, o_sw);
			if(bo_snmp_isChange() == 1) {
				bo_snmp_get_tab();
				bo_prt_switch(i); 
			}
			bo_snmp_unlock_mut();
		}
		dbgout("\n ==================== \n"); 
	}
	
	exit:
	bo_snmp_lock_mut();
	bo_del_snmp();
	bo_del_tab_switch();
	bo_snmp_unlock_mut();
	bo_log("SNMP monitor END");
}

static void bo_checkSwitch(int sock, struct OPT_SWITCH *o_sw) {
	/*  .1.3.6.1.4.1.8691.7.6.1.10.3.1.2 - link
	 *  .1.3.6.1.4.1.8691.7.6.1.10.3.1.3 - speed
	 *  .1.3.6.1.4.1.8691.7.6.1.9.1.1.2  - portName
	 * FIRST ROW:
	 */
	int oid[][14] = { {1, 3, 6, 1, 4, 1, 8691, 7, 6, 1, 10, 3, 1, 2},
	                  {1, 3, 6, 1, 4, 1, 8691, 7, 6, 1, 10, 3, 1, 3},
			  {1, 3, 6, 1, 4, 1, 8691, 7, 6, 1,  9, 1, 1, 2}
			};
	int flag = 0, temp = 0;
	/* NEXT ROW: */
	struct PortItem *portItem = NULL;
	struct PortItem *ports = NULL;
	struct OID_Next *oid_next;
	char *ip = NULL;
	int j = 0;
	
	ip = o_sw->ip;
	bo_snmp_crt_next_req(oid, 3, 14);
	
	ports = o_sw->ports;
	for(j = 0; j < BO_OPT_SW_PORT_N; j++) {
		flag += (ports + j)->flg;
		(ports + j)->flg = -1;
	}
	
	portItem = o_sw->ports;
	oid_next = bo_getPortVal(sock, ip, portItem);
	
	if(oid_next != NULL) {
		bo_snmp_crt_next_req2(oid_next);
		portItem = (o_sw->ports + 1);
		oid_next = bo_getPortVal(sock, ip, portItem);
	}
	
	if(oid_next != NULL) {
		bo_snmp_crt_next_req2(oid_next);
		portItem = (o_sw->ports + 2);
		oid_next = bo_getPortVal(sock, ip, portItem);
	}
	
	if(oid_next != NULL) {
		bo_snmp_crt_next_req2(oid_next);
		portItem = (o_sw->ports + 3);
		oid_next = bo_getPortVal(sock, ip, portItem);
	}
	
	if(oid_next != NULL) {
		bo_snmp_crt_next_req2(oid_next);
		portItem = (o_sw->ports + 4);
		oid_next = bo_getPortVal(sock, ip, portItem);
	}
	
	for(j = 0; j < BO_OPT_SW_PORT_N; j++) {
		temp += (ports + j)->flg;
	}
	if(temp != flag) sw_net_chg = 1;
}

static struct OID_Next *bo_getPortVal(int sock, char *ip, struct PortItem *portItem)
{
	unsigned char *buf  = NULL;
	unsigned char *pack = NULL;
	struct OID_Next *oid_next = NULL;
	int len = 0, exec = 0, port = 0;
	char *ip_recv = NULL;
	fd_set r_set;
	struct timeval tval;
	
	tval.tv_sec  = 0;
	tval.tv_usec = 500000;
	
	portItem->flg = -1;
	
	buf  = bo_snmp_get_buf();
	pack = bo_snmp_get_buf();
	len  = bo_snmp_get_buf_len();
	
	exec = bo_send_udp_packet(sock, pack, len, ip);
	if(exec == -1)  {
		bo_log("getPortVal().bo_send_udp_packet ERROR ip[%s]", ip);
		goto exit;
	}	
	
	FD_ZERO(&r_set);
	FD_SET(sock, &r_set);
	exec = select(sock + 1, &r_set, NULL, NULL, &tval);
	if(exec < 1) {
		dbgout("getPortVal().select don't get event ip[%s]\n", ip);
		goto exit;
	}
	
	exec = bo_recv_udp_packet(buf, 1024, sock, &ip_recv, &port);
	if(exec > 0) {
		if(ip_recv != NULL) {
			if(!strstr(ip_recv, ip)) goto exit;
			exec = bo_parse_oid(buf, exec, portItem, &sw_net_chg);
			if(exec == 1) {
				/*
				dbgout("id[%d] link[%d] speed[%d] descr[%s]\n", portItem->id, 
					portItem->link, 
					portItem->speed, 
					portItem->descr);
				*/
			} else { 
				dbgout("bo_getPortVal() ERROR parse snmp from ip[%s]\n", 
					ip);
				bo_log("bo_getPortVal() ERROR parse snmp from ip[%s]\n", 
					ip); 
				goto exit;
			}

			oid_next = bo_parse_oid_next();
			portItem->flg = 1;
		}
	}
	
	exit:
	return oid_next;
}

static int bo_crt_optSwitch(char *ip[], int n)
{
	int ans = -1, i = 0, j = 0;
	struct OPT_SWITCH *o_sw = NULL;
	struct PortItem *ports = NULL;
	char *s;
	int len;
	if(n < 1) { 
		bo_log("bo_crt_optSwitch() n < 1 can't create tab_sw");
		goto exit;
	}
	tab_sw = (struct OPT_SWITCH *)malloc(n*sizeof(struct OPT_SWITCH));
	if(tab_sw == NULL) {
		bo_log("bo_crt_optSwitch() malloc error[%s]", strerror(errno));
		goto exit;
	}
	
	for(;i < n; i++) {
		o_sw = tab_sw + i;
		s = *(ip + i);
		len = strlen(s);
		if(len > 15) {
			bo_log("bo_crt_optSwitch() bad ip len");
			goto exit;
		}
		memcpy(o_sw->ip, s, strlen(s)+1);
		
		dbgout("ip[%s] len[%d]\n", o_sw->ip, strlen(o_sw->ip));
		
		ports = o_sw->ports;
		for(; j < BO_OPT_SW_PORT_N; j++) {
			(ports + j)->flg = -1;
		}
	}
	
	ans = 1;
	exit:
	return ans;
}

static void bo_del_tab_switch()
{
	if(tab_sw != NULL) free(tab_sw);
}

static void bo_prt_switch(int n)
{
	int i = 0;
	struct OPT_SWITCH *o_sw = (tab_sw + n);
	struct PortItem *port = NULL;
	for(; i < BO_OPT_SW_PORT_N; i++) {
		port = (o_sw->ports + i);
		dbgout("%d:ip[%d][%s] ", i, strlen(o_sw->ip), o_sw->ip );
		dbgout("flg[%d] link[%d] speed[%d] descr[%s]\n", 
			port->flg, port->link, port->speed, port->descr);
	}
}

/* 0x42 */