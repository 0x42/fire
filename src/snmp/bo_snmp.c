#include "bo_snmp.h"

static void bo_snmp_gen_head();
static void bo_print_buf();
static void bo_snmp_gen_getrequest();

const int SNMP_BUF_SIZE = 2048;
static char buf[255];

static struct {
	char *buf;
	int buf_i;
	
	int ver;
        char *com;
        int pdu_type;
        int request_id;
        int error;
        int error_i;
        char *oid;
        int oid_size;
} snmp_core;


/* ----------------------------------------------------------------------------
 * @brief	созд буфер для приема и отправки
 */
int bo_init_snmp()
{
	int ans = 1;
	snmp_core.buf = (char *)malloc(SNMP_BUF_SIZE);
	if(snmp_core.buf == NULL) {
		bo_log("bo_init_snmp() can't alloc memory for buffer");
		ans = -1;
	}
	/* Номер версии SNMP - 1 (RFC 1157) */
	snmp_core.ver		= 0;
	snmp_core.buf_i		= 0;
	return ans;
}

void bo_snmp_crt_msg(char *oid, int size)
{
	snmp_core.pdu_type	= SNMP_GET_REQUEST_TYPE;
	snmp_core.request_id	= 0;
	snmp_core.error		= 0;
	snmp_core.error_i	= 0;
	snmp_core.oid		= oid;
	snmp_core.oid_size	= size;
	snmp_core.buf_i		= 0;
	
	bo_snmp_gen_head();	
	bo_snmp_gen_getrequest();
}

static void bo_snmp_gen_head()
{
	int n_ver = 0, len = 0;
	n_ver = bo_int_size(snmp_core.ver);
	
	*snmp_core.buf = ASN1_INTEGER;
	snmp_core.buf_i++;
	
	bo_code_len( (snmp_core.buf + snmp_core.buf_i) , n_ver);
	len = bo_len_size(n_ver);
	snmp_core.buf_i += len;
	
	
	
//	memcpy( (snmp_core.buf + snmp_core.buf_i), "public", 6);
//	snmp_core.buf_i += 6;
	
	bo_print_buf();
}

static void bo_snmp_gen_getrequest()
{
	int i = 0, len = 0;
	snmp_core.request_id++;
	if(snmp_core.request_id == 100000) snmp_core.request_id++;
	len = bo_int_size(snmp_core.request_id);
	*buf = ASN1_INTEGER;
	bo_code_len(buf + 1, len);
	
}

void bo_del_snmp()
{
	if(snmp_core.buf != NULL) free(snmp_core.buf);
}

static void bo_print_buf()
{
	int i = 0;
	printf("snmp_core.buf [");
	for(; i < snmp_core.buf_i; i++) {
		printf("%02x", *(snmp_core.buf + i) );
	}
	printf("]\n");
}
/* 0x42 */