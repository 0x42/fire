#include "bo_parse.h"

static void snmpReadHead	(struct PortItem*);
static void snmpReadId		(struct PortItem*);
static void snmpReadErr		(struct PortItem*);
static void snmpReadVarBind	(struct PortItem*);
static void snmpQuit		(struct PortItem*);
static void snmpErr		(struct PortItem*);

static int bo_parse_VALLINK (struct PortItem *p);
static int bo_parse_VALSPEED(struct PortItem *p);
static int bo_parse_VALDESCR(struct PortItem *p);

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

static struct OID_Next oid_next;
/* ----------------------------------------------------------------------------
 * @return	[-1] ERROR [1] OK	
 */
int bo_parse_oid(unsigned char *snmp, int len, struct PortItem *port)
{
	int ans   = 1;
	PS.snmp   = snmp;
	PS.len    = len;
	PS.status = READHEAD;
	PS.ptr    = 0;
	
	while(1) {
		dbgout("KA[%s]\n", ParseStatusTxt[PS.status]);
		if(PS.status == ERR) ans = -1;
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
	int id = 0, exec;
	unsigned char *temp = NULL;
	
	temp = PS.snmp + PS.ptr;

	if( *temp == SNMP_GET_RESPONSE_TYPE) {
		PS.ptr++;
		temp = PS.snmp + PS.ptr;
		resp_len_size = bo_len_ber_size(temp);
		PS.ptr += resp_len_size;
		
		temp = PS.snmp + PS.ptr;
		exec = bo_parse_INTEGER(temp, &id);
		if(exec == -1) {
			bo_log("snmpReadId can't parse ID");
			goto error_label;
		}
		
		p->id = id;
		PS.ptr += exec;
	} else {
		bo_log("snmpReadId() bad snmp format");
		goto error_label;
	}
	
	PS.status = READERR;
	if(error == -1) {
		error_label:
		PS.status = ERR;
	}
}

static void snmpReadVarBind(struct PortItem *p)
{
	unsigned char *temp = NULL;
	int len = 0, exec = 0;
	const int error = 1;
	
	if( *PS.snmp == ASN1_SEQUENCE) {
		PS.ptr++;
		temp = PS.snmp + PS.ptr;
		
		len = bo_len_ber_size(temp);
		PS.ptr += len;
		
		exec = bo_parse_VALLINK(p);
		if(exec == -1) {
			bo_log("snmpReadVarBing() ERROR");
			goto error_label;
		}
		
		/* PORT SPEED */
		exec = bo_parse_VALSPEED(p);
		if(exec == -1) {
			bo_log("snmpReadVarBing() ERROR");
			goto error_label;
		}
		
		/* PORT DESCREPTION */
		exec = bo_parse_VALDESCR(p);
		if(exec == -1) {
			bo_log("snmpReadVarBing() ERROR");
			goto error_label;
		}
	} else {
		bo_log("snmpReadVarBind() error(when read link)");
		goto error_label;
	}
	
	PS.status = QUIT;
	if(error == -1) {
		error_label:
		PS.status = ERR;
	}
}

static void snmpReadErr(struct PortItem *p) 
{
	int err = 0, err_index = 0;
	int len;
	const int error = 1;
	unsigned char *temp = NULL;
	char *err_txt[5] = {
		"TOO_BIG", 
		"NO_SUCH_NAME", 
		"BAD_VALUE", 
		"READ_ONLY", 
		"GEN_ERR"
	};
	
	temp = PS.snmp + PS.ptr;
	len = bo_parse_INTEGER(temp, &err);

	if(len == -1) {
		bo_log("snmpReadErr() can't parse field error");
		goto error_label;
	}
	
	PS.ptr += len;
	temp = PS.snmp + PS.ptr;
	len = bo_parse_INTEGER(temp, &err_index);
	
	if(err != 0) {
		if( (err-1) < 5) {
			bo_log("SNMP ERROR[%s] ERROR_INDEX[%d]", 
			err_txt[err], err_index);
		} else 	bo_log("SNMP ERROR[%d] ERROR_INDEX[%d]", 
			err, err_index);
		goto error_label;
	}
	
	PS.ptr += len;
	PS.status = READVARBIND;
	if(error == -1) {
		error_label:
		PS.status = ERR;
	}
}

static void snmpErr(struct PortItem *p)
{
	dbgout("snmpErr\n");
	PS.status = QUIT;
}

static void snmpQuit(struct PortItem *p)
{
	dbgout("snmpQuit\n");
}

/* =============================== END STATE MACHINE ======================== */
/* ----------------------------------------------------------------------------
 * @brief	parse OID:VALUE_LINK
 * @return	[-1] ERROR [1] OK
 */
static int bo_parse_VALLINK(struct PortItem *p)
{
	int ans = -1, exec = 0, value_len = 0;
	int link = 0;
	unsigned char *temp = NULL;
	
	temp = PS.snmp + PS.ptr;
	exec = bo_parse_OID(temp, 
			oid_next.link, 
			&oid_next.link_size, 
			&value_len);
	if(exec == -1) {
		bo_log("bo_parse_VALLINK() ERROR parse oid link");
		goto exit;
	}
	PS.ptr += exec;
	temp = PS.snmp + PS.ptr;
	exec = bo_parse_INTEGER(temp, &link);
	if(exec == -1) {
		bo_log("bo_parse_VALLINK() ERROR parse link value");
		goto exit;
	}
	PS.ptr += exec;
	p->link = link;
	ans = 1;
	
	exit:
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	parse OID:VALUE_SPEED
 * @return	[-1] ERROR [1] OK
 */
static int bo_parse_VALSPEED(struct PortItem *p)
{
	int ans = -1, exec = 0, value_len = 0;
	int speed = 0;
	unsigned char *temp = NULL;
	
	temp = PS.snmp + PS.ptr;
	exec = bo_parse_OID(temp, 
			oid_next.speed, 
			&oid_next.speed_size, 
			&value_len);
	if(exec == -1) {
		bo_log("bo_parse_VALSPEED() ERROR parse oid speed");
		goto exit;
	}
	PS.ptr += exec;
	temp = PS.snmp + PS.ptr;
	exec = bo_parse_INTEGER(temp, &speed);
	if(exec == -1) {
		bo_log("bo_parse_VALSPEED() ERROR parse speed value");
		goto exit;
	}
	PS.ptr += exec;
	p->speed = speed;
	ans = 1;
	
	exit:
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	parse OID:VALUE_DESCR
 * @return	[-1] ERROR [1] OK
 */
static int bo_parse_VALDESCR(struct PortItem *p)
{
	int ans = -1, exec = 0, value_len = 0;
	unsigned char *temp = NULL;
	
	temp = PS.snmp + PS.ptr;
	exec = bo_parse_OID(temp,
			oid_next.descr,
			&oid_next.descr_size,
			&value_len);
	if(exec == -1) {
		bo_log("bo_parse_VALDESCR() ERROR parse oid descr");
		goto exit;
	}
	PS.ptr += exec;
	temp = PS.snmp + PS.ptr;
	memset(p->descr, 0, PORTITEM_DESCR);
	exec = bo_parse_STRING(temp, p->descr, &p->descr_len);
	if(exec == -1) {
		bo_log("bo_parse_VALDESCR() ERROR parse descr value");
		goto exit;
	}
	PS.ptr += exec;
	ans = 1;
	
	exit:
	return ans;
}

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
		*num = bo_uncode_int(temp, len);
		ptr += len;
		ans = ptr;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	parse OID
 * @value_len	возв размер поля VALUE
 * @return	[-1] ERROR [N BYTE] OK 
 */
int bo_parse_OID(unsigned char *buf, 
	        int oid[20], 
	        int *arr_len, 
		int *value_len)
{
	int len = -1, ptr = 0, n = 0, i = 0;
	unsigned int t = 0;
	int oid_len = 0, seq_len = 0;
	unsigned char *temp = NULL;
	
	if( *buf == ASN1_SEQUENCE) {
		len = 0;
		ptr++;
		len++;
		
		seq_len = bo_uncode_len((buf + ptr));
		len+= seq_len;
	
		t += bo_len_ber_size( (buf + ptr) );		
		len += t;
		ptr += t;
		temp = buf + ptr;
		if(*temp == OID_TYPE) {
			ptr++;
			/* Размер OID */
			oid_len = bo_uncode_len((buf + ptr));
			ptr += bo_len_ber_size( (buf + ptr) );
			
			/* Размер VALUE */
			*value_len = seq_len - oid_len - 2;
			temp = buf + ptr;

			/* Parse OID */
			oid[0] = 1; oid[1] = 3;				
			/* первый oid пропускаем он всегда равен
			   2b, вместо первых двух oid 1 3 
			 */
			temp++; 
			i = 2;
			while(n < (oid_len-1) ) {
				t = 0;
				n += bo_uncode_oid( (temp+n), &t);
				oid[i] = t;
				i++;
			}
			*arr_len = i;
		}
		len = len - (*value_len);
	} 
	
	return len;
}

/* ----------------------------------------------------------------------------
 * @brief	parse STRING 
 * @return	[-1] ERROR [N BYTE] OK
 */
int bo_parse_STRING(unsigned char *buf,
		    char *str,
		    int *str_len)
{
	int ans = -1, ptr = 0, len = 0, i = 0;
	unsigned char *temp = NULL;
	
	if(*buf == ASN1_STRING) {
		ans = 0;
		ptr++;
		temp = buf + ptr;
		len = bo_uncode_len(temp);
		printf("len[%d]\n",len);
		ptr++;
		temp = buf + ptr;
		if(len > 20) len = 20;
		for(i = 0; i < len; i++) {
			*(str + i) = *(temp + i);
		}
		*str_len = len;
		ans += ptr + len; 
	}
	
	return ans;
}

struct OID_Next *bo_parse_oid_next()
{
	return &oid_next;
}


/* 0x42 */