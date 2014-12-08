#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../log/bologging.h"
#include "../tools/dbgout.h"
#include "bo_asn.h"
#include "bo_snmp.h"
#include "../netudp/bo_udp.h"

void bo_snmp_main()
{
	int exec = -1;
	int oid_name[] = {1, 3, 6, 1, 2, 1, 1, 3, 0};
	int oid_sysName[] = {1, 3, 6, 1, 2, 1, 1, 5, 0};
	int oid_tab_i[] = {1, 3, 6, 1, 4, 1, 8691, 7, 6, 1, 1, 0};
	dbgout("bo_main_snmp ... run\n");
	unsigned char *pack;
	unsigned char *buf;
	char *ip;
	int port;
	
	int len;
	exec = bo_init_snmp();
	if(exec == -1) bo_log("bo_snmp_main() ERROR can't create data for run snmp");
	
//	bo_snmp_crt_msg(oid_tab_i, 12);
	
	bo_snmp_crt_msg(oid_sysName, 9);

	int sock = bo_udp_sock();
	buf = bo_snmp_get_buf();
	pack = bo_snmp_get_msg();
	len  = bo_snmp_get_msg_len();
	exec = bo_send_udp_packet(sock, pack, len, "192.168.1.151");
	if(exec == -1)	printf("send udp error\n");
	
	sleep(1);
	
	exec = bo_recv_udp_packet(buf, 1024, sock, &ip, &port);
	
	int i = 0;
	printf("recv buf[");
	for(; i < exec; i++) {
		printf("%02x ", *(buf + i) );
	}
	if(ip != NULL) printf("]\n ip[%s]\n", ip);
	else printf("]\n");
	
	bo_del_snmp();
	dbgout("\nbo_main_snmp ... end\n");
}