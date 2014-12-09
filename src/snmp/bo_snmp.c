#include "bo_snmp.h"

static void bo_snmp_gen_head();
static void bo_print_buf();
static  int bo_snmp_gen_getrequest_head();
static void bo_snmp_gen_getrequest();
static void bo_ber_header(unsigned char type, int len);

static int bo_ber_head(unsigned char type, int len, unsigned char *buf);
static int bo_gen_varbind(unsigned char *buf, int *oid, int size);
static int bo_gen_ver_comm(unsigned char *buf);
static int bo_gen_pdu_head(unsigned char *buf);

const int SNMP_BUF_SIZE = 1024;
static const int BSIZE = 255;
static unsigned char buf[255];
 
static struct {
	unsigned char *buf;
	unsigned char *pdu;
	int buf_i;
	int pdu_i;
	
	int ver;
        char *com;
        int pdu_type;
        int request_id;
        int error;
        int error_i;
        int oid_size;
	int *oid_i;
} snmp_core;


/* ----------------------------------------------------------------------------
 * @brief	созд буфер для приема и отправки
 * @return	[1] OK [-1] ERROR 
 */
int bo_init_snmp()
{
	int ans = 1;
	snmp_core.buf = (unsigned char *)malloc(SNMP_BUF_SIZE);
	if(snmp_core.buf == NULL) {
		bo_log("bo_init_snmp() can't alloc memory for buf");
		ans = -1;
		goto exit;
	}
	snmp_core.pdu = (unsigned char *)malloc(SNMP_BUF_SIZE);
	if(snmp_core.pdu == NULL) {
		bo_log("bo_init_snmp() can't alloc memory for pdu");
		ans = -1;
		free(snmp_core.buf);
		goto exit;
	}
	
	/* Номер версии SNMP - 1 (RFC 1157) */
	snmp_core.ver		= 0;
	snmp_core.buf_i		= 0;
	
	exit:
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

void bo_snmp_crt_next_req(int oid[][14], int n, int m)
{
	int i = 0, j = 0, exec = 0;
	unsigned char *temp = buf;
	int buf_len = 0;
	int req_head_len = 0,
	    varbind_len  = 0,
	    all_len      = 0,
	    t = 0;
	
	snmp_core.buf_i = 0;
	
	for(; i < n; i++) {
		exec = bo_gen_varbind(temp, oid[i], m);
		buf_len += exec;
		printf("exec[%d]\n", exec);
		temp = buf + buf_len;
	}
	
	t = bo_int_size(snmp_core.request_id) + 8;
	
	varbind_len = 1 + bo_len_size(buf_len) + buf_len;
	req_head_len = varbind_len + t;
	all_len = req_head_len + 12 + bo_len_size(req_head_len);
		
	snmp_core.buf_i += bo_ber_head(ASN1_SEQUENCE, 
				       all_len, 
				       snmp_core.buf);
	
	temp = snmp_core.buf + snmp_core.buf_i;
	snmp_core.buf_i += bo_gen_ver_comm(temp);
	
	temp = snmp_core.buf + snmp_core.buf_i;
	snmp_core.buf_i += bo_ber_head(SNMP_GET_NEXT_REQUEST_TYPE,
				       req_head_len,
				       temp);
	
	temp = snmp_core.buf + snmp_core.buf_i;
	snmp_core.buf_i += bo_gen_pdu_head(temp);
	
	temp = snmp_core.buf + snmp_core.buf_i;
	snmp_core.buf_i += bo_ber_head(ASN1_SEQUENCE, 
				       buf_len,
				       temp);
	
	temp = snmp_core.buf + snmp_core.buf_i;
	memcpy(temp, buf, buf_len);
	snmp_core.buf_i += buf_len;
	
	printf("snmp[\n");
	int eol = 0;
	for(i = 0; i < snmp_core.buf_i; i++) {
		if(eol == 8) {eol = 0; printf("\n");}
		printf("%02x ", *(snmp_core.buf + i) );
		eol++;
	}
	printf("]\n");
	/*
	printf("buf[");
	for(i = 0; i < buf_len; i++) {
		printf("%02x", *(buf+i) );
	}
	printf("]\n");
	printf("buf_len = [%d]\n", buf_len);
	*/
}

unsigned char * bo_snmp_get_msg()
{
	return snmp_core.pdu;
}

unsigned char * bo_snmp_get_buf()
{
	return snmp_core.buf;
}

int bo_snmp_get_msg_len()
{
	return snmp_core.pdu_i;
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
	len = bo_int_size(snmp_core.request_id);  
	*buf = ASN1_INTEGER;
	i++;

	bo_code_len(buf + i, len);
	i += bo_len_size(len);
	
	i += bo_code_int(buf + i, snmp_core.request_id);

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
	req_head = bo_snmp_gen_getrequest_head();

	/* Считаем длину сообщения */
	oid_len = bo_oid_length(snmp_core.oid_i, snmp_core.oid_size);
	
	var = oid_len + bo_len_size(oid_len) + 1;
	seq1 = var + bo_len_size(var + 2) + 1;	
	seq2 = seq1 + bo_len_size(seq1) + 1;

	req_len = req_head;	
	req_len += seq2 + bo_len_size(seq2) + 1;

	/* Формируем сообщение */
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
	printf("buf_i[%d]\n", snmp_core.buf_i);
	
	*snmp_core.pdu = ASN1_SEQUENCE;
	snmp_core.pdu_i = 1;
	bo_code_len( (snmp_core.pdu + snmp_core.pdu_i), snmp_core.buf_i);
	snmp_core.pdu_i += bo_len_size(snmp_core.buf_i);
	
	memcpy( (snmp_core.pdu + snmp_core.pdu_i), snmp_core.buf, snmp_core.buf_i);
	snmp_core.pdu_i += snmp_core.buf_i;
	
	snmp_core.buf_i = 0;
}

/* ----------------------------------------------------------------------------
 * @brief	Создание поля элемента VARBIND
 *		|SEQ|LEN_SEQ|VAR|OID_LEN|OID + 05 00
 */
static int bo_gen_varbind(unsigned char *buf, int *oid, int size)
{
	int ptr      = 0,
	    i        = 0, 
	    oid_len  = 0,
	    var      = 0,
	    seq_len  = 0;
	
	unsigned char *temp = NULL;
	
	oid_len = bo_oid_length(oid, size);
	var	= oid_len + bo_len_size(oid_len) + 1;
	seq_len = var + 2; /* 05 00 (2bytes) OID value*/
	
	ptr += bo_ber_head(ASN1_SEQUENCE, seq_len, buf);
	
	temp = buf + ptr;
	ptr += bo_ber_head(OID_TYPE, oid_len, temp);
	
	temp = buf + ptr;
	*temp = 0x2b;
	ptr++;
	
	/* формируем поле name:value
	 * name =  OID; value = NULL */
	for(i = 2; i < size; i++) {
		temp = buf + ptr;
		ptr += bo_code_oid(*(oid + i), temp);
	}
	
	temp = buf + ptr;
	*temp = 0x05;
	ptr++;
	
	temp = buf + ptr;
	*temp = 0x00;
	ptr++;
	
	return ptr;
}

static int bo_ber_head(unsigned char type, int len, unsigned char *buf)
{
	int ptr = 0;
	unsigned char *b = NULL;
	
	*buf = type;
	ptr++;
	b = (buf + ptr);
	bo_code_len(b, len);
	ptr += bo_len_size(len);
	return ptr;
}

/* ----------------------------------------------------------------------------
 * @brief	генер-ем VERSION|COMMUNITY
 * @return	n - кол-во байт занимает инф 
 */
static int bo_gen_ver_comm(unsigned char *buf)
{
	int ptr = 0;
	unsigned char *temp = NULL;
	
	/* VERSION */
	*buf = ASN1_INTEGER;
	*(buf + 1) = 1;
	*(buf + 2) = 0;
	ptr += 3;
	
	/* COMMUNITY */
	temp = buf + ptr;
	ptr += bo_code_string(temp, (unsigned char *)"public", 6);
	
	return ptr;
}

/* ----------------------------------------------------------------------------
 * @brief	генер-ем REQUEST-ID|ERR-STATUS|ERROR-INDEX
 */
static int bo_gen_pdu_head(unsigned char *buf)
{
	int ptr = 0, len = 0;
	
	snmp_core.request_id++;
	if(snmp_core.request_id == 100000) snmp_core.request_id++;
	len = bo_int_size(snmp_core.request_id);  
	*buf = ASN1_INTEGER;
	ptr++;

	bo_code_len(buf + ptr, len);
	ptr += bo_len_size(len);
	
	ptr += bo_code_int(buf + ptr, snmp_core.request_id);

	 /*DOC: error status always 0 */
	*(buf + ptr) = ASN1_INTEGER; ptr++;
	*(buf + ptr) = 1;	     ptr++;
	*(buf + ptr) = 0;	     ptr++;
	/*DOC: error index always 0 */
	*(buf + ptr) = ASN1_INTEGER; ptr++;
	*(buf + ptr) = 1;	     ptr++;
	*(buf + ptr) = 0;	     ptr++;
	
	return ptr;
}

/* ----------------------------------------------------------------------------
 * @brief	созд заголовок 
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
	if(snmp_core.pdu != NULL) free(snmp_core.pdu);
}

static void bo_print_buf()
{
	int i = 0;
	printf("snmp_core.pdu [");
	for(; i < snmp_core.pdu_i; i++) {
		printf("%02x ", *(snmp_core.pdu + i) );
	}
	printf("]\n");
}
/* 0x42 */