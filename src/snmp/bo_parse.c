#include "bo_parse.h"

static void snmpReadHead	(struct PortItem*);
static void snmpReadId		(struct PortItem*);
static void snmpReadErr		(struct PortItem*);
static void snmpReadVarBind	(struct PortItem*);
static void snmpQuit		(struct PortItem*);
static void snmpErr		(struct PortItem*);

static char *ParseStatusTxt[] = {"READHEAD", 
		    		 "READID", 
				 "READERR", 
				 "READVARBIND", 
				 "QUIT",
				 "ERR"};

enum ParseStatus {READHEAD = 0,
		  READID,
		  READERR,
		  READVARBIND,
		  QUIT,
		  ERR};

static void(*statusTable[])(struct PortItem *) = {
	snmpReadHead,
	snmpReadId,
	snmpReadErr,
	snmpReadVarBind,
	snmpQuit,
	snmpErr
};			 

static struct ParseState {
	unsigned char *snmp;
	int len;
	int ptr;
	enum ParseStatus status;
} PS;

/* ----------------------------------------------------------------------------
 * @return	[-1] ERROR [>0] requestId	
 */
int bo_parse_oid(unsigned char *snmp, int len, struct PortItem *port)
{
	int ans   = -1;
	PS.snmp   = snmp;
	PS.len    = len;
	PS.status = READHEAD;
	PS.ptr    = 0;
	
	while(1) {
		if(PS.status == QUIT) break;
		statusTable[PS.status](port);
	}
	
	return ans;
}

static void snmpReadHead(struct PortItem *p) 
{
	const int error = 1;
	int snmp_len_size = 0;
	unsigned char *temp;
	
	printf("snmpReadHead\n");
	if( *PS.snmp == ASN1_SEQUENCE) {
		PS.ptr++;
		temp = PS.snmp + PS.ptr;
		
		/* размер поля MESSAGE LENGTH */
		snmp_len_size = bo_len_ber_size(temp);
		PS.ptr += 11 + snmp_len_size;
		
		/* смещаемся до GET-RESPONSE */
		if(PS.ptr >= PS.len) {
			bo_log("snmpReadHead() bad snmp msg length");
			goto error_label;
		}
	} else {
		bo_log("snmpReadHead() bad snmp format");
		goto error_label;
	}
	
	PS.status = READID;
	if(error == -1) {
		error_label:
		PS.status = ERR;
	}
	
}

static void snmpReadId(struct PortItem *p) 
{
	const int error = 1; 
	int resp_len_size = 0;
	int id = 0;
	unsigned char *temp = NULL;
	
	printf("snmpReadId\n");
	
	temp = PS.snmp + PS.ptr;

	if( *temp == SNMP_GET_RESPONSE_TYPE) {
		PS.ptr++;
		temp = PS.snmp + PS.ptr;
		resp_len_size = bo_len_ber_size(temp);
		PS.ptr += resp_len_size;
		
		temp = PS.snmp + PS.ptr;
		id = bo_uncode_int(temp);
		
	} else {
		bo_log("snmpReadId() bad snmp format");
		goto error_label;
	}
	
	PS.status = QUIT;
	if(error == -1) {
		error_label:
		PS.status = ERR;
	}
}

static void snmpReadVarBind(struct PortItem *p)
{
	printf("snmpReadVarBind\n");
	PS.status = QUIT;
}

static void snmpReadErr(struct PortItem *p) 
{
	printf("snmpReadErr\n");
	PS.status = QUIT;
}

static void snmpErr(struct PortItem *p)
{
	printf("snmpErr\n");
	PS.status = QUIT;
}

static void snmpQuit(struct PortItem *p)
{
	printf("snmpQuit\n");
}

/* ========================================================================== */
/* ----------------------------------------------------------------------------
 * @brief	 parse INTEGER
 * @num		 answer
 * @return	 [-1] ERROR [N BYTE]
 */
int bo_parse_INTEGER(unsigned char *buf, int *num)
{
	int ans = -1, ptr = 0;
	unsigned int len = 0;
	unsigned char *temp = NULL;
	
	if(*buf == ASN1_INTEGER) {
		ptr++;
		temp = buf + ptr;
		len = bo_uncode_len(temp);
		
		ptr++;
		temp = buf + ptr;
		
	}
	return ans;
}

/* 0x42 */