#include "bo_snmp.h"

static void bo_snmp_gen_head();
static void bo_print_buf();
static  int bo_snmp_gen_getrequest_head();
static void bo_snmp_gen_getrequest();
static void bo_ber_header(unsigned char type, int len);

const int SNMP_BUF_SIZE = 2048;
static const int BSIZE = 255;
static unsigned char buf[255];
 
static struct {
	unsigned char *buf;
	int buf_i;
	
	int ver;
        char *com;
        int pdu_type;
        int request_id;
        int error;
        int error_i;
        char *oid;
        int oid_size;
	int *oid_i;
} snmp_core;


/* ----------------------------------------------------------------------------
 * @brief	созд буфер для приема и отправки
 */
int bo_init_snmp()
{
	int ans = 1;
	snmp_core.buf = (unsigned char *)malloc(SNMP_BUF_SIZE);
	if(snmp_core.buf == NULL) {
		bo_log("bo_init_snmp() can't alloc memory for buffer");
		ans = -1;
	}
	/* Номер версии SNMP - 1 (RFC 1157) */
	snmp_core.ver		= 0;
	snmp_core.buf_i		= 0;
	return ans;
}

void bo_snmp_crt_msg(int *oid, int size)
{
	snmp_core.pdu_type	= SNMP_GET_REQUEST_TYPE;
	snmp_core.request_id	= 0;
	snmp_core.error		= 0;
	snmp_core.error_i	= 0;
	snmp_core.oid_i		= oid;
	snmp_core.oid_size	= size;
	
	bo_snmp_gen_head();	
	bo_snmp_gen_getrequest();
	
	bo_print_buf();
}

static void bo_snmp_gen_head()
{
	int n_ver = 0, len = 0;
	n_ver = bo_int_size(snmp_core.ver);
	/* тип блока */
	*snmp_core.buf = ASN1_INTEGER;
	snmp_core.buf_i++;
	
	/* длина блока*/
	bo_code_len( (unsigned char *)(snmp_core.buf + snmp_core.buf_i) , n_ver);
	len = bo_len_size(n_ver);
	snmp_core.buf_i += len;
	
	/* значение */
	*(snmp_core.buf + snmp_core.buf_i) = snmp_core.ver;
	snmp_core.buf_i++;
	
	len = bo_code_string(
		(unsigned char *)(snmp_core.buf + snmp_core.buf_i), 
		(unsigned char *)"public", 
		6);
	snmp_core.buf_i += len;
}

static int bo_snmp_gen_getrequest_head()
{
	int i = 0, len = 0;
	
	snmp_core.request_id++;
	if(snmp_core.request_id == 100000) snmp_core.request_id++;
/*	len = bo_int_size(snmp_core.request_id);  < -- уточнить 0x42 */
	
	len = 4; /* integer always 4 bytes*/
	*buf = ASN1_INTEGER;
	i++;
	
	bo_code_len(buf + i, len);
	i += bo_len_size(len);
	
	bo_code_int(buf + i, snmp_core.request_id);
	i+= 4;
	/* error status always 0 */
	*(buf + i) = ASN1_INTEGER; i++;
	*(buf + i) = 1;		   i++;
	*(buf + i) = 0;		   i++;
	/* error index always 0*/
	*(buf + i) = ASN1_INTEGER; i++;
	*(buf + i) = 1;		   i++;
	*(buf + i) = 0;		   i++;
	return i;
}

static void bo_snmp_gen_getrequest()
{
	int req_head = 0, oid_len, var, seq1, seq2, i;
	int req_len;
	unsigned char *b;
	req_head = bo_snmp_gen_getrequest_head();
	
	oid_len = bo_oid_length(snmp_core.oid_i, snmp_core.oid_size);
	
	var = oid_len + bo_len_size(oid_len) + 1;
	seq1 = var + bo_len_size(var + 2) + 1;	
	seq2 = seq1 + bo_len_size(seq1) + 1;

	req_len = req_head;	
	req_len += seq2 + bo_len_size(seq2) + 1;
	
	bo_ber_header(GET_REQ_PDU, req_len);

	memcpy( (snmp_core.buf + snmp_core.buf_i), buf, req_head);
	snmp_core.buf_i += req_head;
	
	bo_ber_header(ASN1_SEQUENCE, seq2);
		
	bo_ber_header(ASN1_SEQUENCE, seq1);
		
	bo_ber_header(OID_TYPE, oid_len);
		
	*(snmp_core.buf + snmp_core.buf_i) = 0x2b;
	snmp_core.buf_i++;
	
	for(i = 2; i < snmp_core.oid_size; i++) {
		var = bo_code_oid( 
			*(snmp_core.oid_i + i),
			(snmp_core.buf + snmp_core.buf_i));
		snmp_core.buf_i += var;
	}
	
	*(snmp_core.buf + snmp_core.buf_i) = 0x05;
	snmp_core.buf_i++;
	
	*(snmp_core.buf + snmp_core.buf_i) = 0x00;
	snmp_core.buf_i++;
	
}

/* ----------------------------------------------------------------------------
 * @brief  созд заголовок 
 */
static void bo_ber_header(unsigned char type, int len)
{
	unsigned char *b = NULL;
	*(snmp_core.buf + snmp_core.buf_i) = type;
	snmp_core.buf_i++;
	b = (unsigned char *)(snmp_core.buf + snmp_core.buf_i);
	bo_code_len( b, len);
	snmp_core.buf_i += bo_len_size(len);
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
		printf("%02x ", *(snmp_core.buf + i) );
	}
	printf("]\n");
}
/* 0x42 */