#include "bo_snmp.h"

static struct SNMP_MSG snmp_msg;

void bo_snmp_crt_msg(unsigned char *buf, 
		     int bufSize)
{
	snmp_msg.ver = 0;
	
}
