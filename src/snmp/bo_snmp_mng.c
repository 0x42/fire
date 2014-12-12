#include "bo_snmp_mng.h"

static struct OID_Next *getPortVal(int sock, char *ip, struct PortItem *portItem);
static void bo_checkSwitch(int sock, struct OPT_SWITCH *o_sw);

void bo_snmp_main(char *ip, int n)
{
	int exec = -1, sock = -1;
	struct OPT_SWITCH *o_sw = NULL;
	
	exec = bo_init_snmp();
	if(exec == -1) { 
		bo_log("bo_snmp_main ERROR can't create data for run snmp");
		goto exit;
	}
	sock = bo_udp_sock();
	if(sock == 0) {
		bo_log("bo_snmp_main ERROR can't create socket for run snmp");
		goto exit;
	}
	
	bo_checkSwitch(sock, o_sw);
	
	exit:
	bo_del_snmp();
}

static void bo_checkSwitch(int sock, struct OPT_SWITCH *o_sw) {
	int exec = -1;
	/*  .1.3.6.1.4.1.8691.7.6.1.10.3.1.2 - link
	 *  .1.3.6.1.4.1.8691.7.6.1.10.3.1.3 - speed
	 *  .1.3.6.1.4.1.8691.7.6.1.9.1.1.2  - portName
	 * FIRST ROW:
	 */
	int oid[][14] = { {1, 3, 6, 1, 4, 1, 8691, 7, 6, 1, 10, 3, 1, 2},
	                  {1, 3, 6, 1, 4, 1, 8691, 7, 6, 1, 10, 3, 1, 3},
			  {1, 3, 6, 1, 4, 1, 8691, 7, 6, 1,  9, 1, 1, 2}
			};
	/* NEXT ROW: */
	dbgout("bo_checkSwitch ... run\n");
	struct PortItem portItem;
	struct OID_Next *oid_next;

	bo_snmp_crt_next_req(oid, 3, 14);
	oid_next = getPortVal(sock, "192.168.1.151", &portItem);
	if(oid_next != NULL) {
		bo_snmp_crt_next_req2(oid_next);
		oid_next = getPortVal(sock, "192.168.1.151", &portItem);
	}
	
	if(oid_next != NULL) {
		bo_snmp_crt_next_req2(oid_next);
		oid_next = getPortVal(sock, "192.168.1.151", &portItem);
	}
	
	if(oid_next != NULL) {
		bo_snmp_crt_next_req2(oid_next);
		oid_next = getPortVal(sock, "192.168.1.151", &portItem);
	}
	
	if(oid_next != NULL) {
		bo_snmp_crt_next_req2(oid_next);
		oid_next = getPortVal(sock, "192.168.1.151", &portItem);
	}
}

static struct OID_Next *getPortVal(int sock, char *ip, struct PortItem *portItem)
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
	exec = select(sock+1, &r_set, NULL, NULL, &tval);
	if(exec < 1) {
		dbgout("getPortVal().select don't get event");
		goto exit;
	}
	
	exec = bo_recv_udp_packet(buf, 1024, sock, &ip_recv, &port);
	if(exec > 0) {
		if(ip_recv != NULL) {
			if(!strstr(ip_recv, ip)) goto exit;
			exec = bo_parse_oid(buf, exec, portItem);
			if(exec == 1) {
				dbgout("id[%d] link[%d] speed[%d] descr[%s]\n", portItem->id, 
					portItem->link, 
					portItem->speed, 
					portItem->descr);
			} else { dbgout("error parse ip[%s]\n", ip_recv); }

			oid_next = bo_parse_oid_next();
			portItem->flg = 1;
		}
	}
	
	exit:
	return oid_next;
}

/* 0x42 */