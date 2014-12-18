/* ----------------------------------------------------------------------------
 * Реализация стандарта ASN.1 (необходим для SNMP)
 * ---------------------------------------------------------------------------- 
 */
#include "bo_asn.h"

/* ----------------------------------------------------------------------------
 * @brief   вычисл колво байт для хранения длины примитивов ASN
 * 2   = 00000010 return 1
 * 128 = 10000001 10000000 return 2
 * 256 = 10000010 00000001 00000000 return 3
 */
int bo_len_size (int len) {
	int ans = -1;
	if(len < 128) ans = 1;
	else {
		ans = 2;
		while (len > 255)
		{
		    len >>= 8;
		    ans += 1;
		}
            
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	кодируем len согласно BER(http://en.wikipedia.org/wiki/X.690)
 * @len_ber	массив куда будет записан рез-тат
 * @len
 */
void bo_code_len(unsigned char *len_ber, int len) 
{
	int i = 0, n = 0;
	
	n = bo_len_size(len);
	if(n == 1) {
		*len_ber = (unsigned int)len;
	} else {
		n--;
		*len_ber = (unsigned int)(128 + n); 
		for(i = n; i != 0; i--) {
			*(len_ber + i) = (unsigned int)len & 0xff;
			len >>=8;
		}
	}
}

/* ----------------------------------------------------------------------------
 * @return	кол-во байт из которых состоит поле BER(Length)
 */
int bo_len_ber_size (unsigned char *len_ber)
{
	int n = 0, ans = 0;
	n = *(unsigned char *)len_ber;
	
	if(n < 128) ans = 1;
	else {
		/* удаляем старший бит согласно BER */
		n = n ^ 0x80;
		ans = (unsigned int)n;
		ans++;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	читаем длину из закод поля
 * @len_ber	поле Length закод-ое по правилу BER		
 */
unsigned int bo_uncode_len(unsigned char *len_ber)
{
	unsigned int len = 0, i = 0, size = 0;
	unsigned char buf;
	
	size = bo_len_ber_size(len_ber);
	
	if(size == 1) {
		len = *(unsigned char *)len_ber;
	} else {
		for(i = 1; i < size; i++) {
			buf = (unsigned char)*(len_ber + i);
			len = len |  (unsigned int)buf;
		}
	}
	
	return len;
}

/* ----------------------------------------------------------------------------
 * @brief	кол-во байт необх-мые для хранения INTEGER
 */
int bo_int_size(int num)
{
	int len = 0, i = 0;
	int hb = 0;
	
	i = (num >> 24) & 0xFF;
	if(i > 0 ) {
		len++;
		hb = 1;
	}
	
	i = (num >> 16) & 0xFF;
	if(hb == 1 ) {
		len++;
	} else if( i > 0) {
		if(i > 127) {
			len++;
		}
		len++;
		hb = 1;
	}
	
	i = (num >> 8) & 0xFF;
	if(hb == 1 ) {
		len++;
	} else if( i > 0) {
		if(i > 127) {
			len++;
		}
		len++;
		hb = 1;
	}

	i = num & 0xFF;
	if( (i > 127) & (hb == 0) ) {
		len++;
	}
	len++;

	return len;
}

/* ----------------------------------------------------------------------------
 * @brief	кодируем INTEGER(раск-ем по основанию 256)
 * @return	размер buf	
 * max int 7F FF FF FF
 */
int bo_code_int(unsigned char *buf, int num)
{
	int len = 0, i = 0;
	int hb = 0;
	
	i = (num >> 24) & 0xFF;
	if(i > 0 ) {
		*buf = (unsigned char ) i;
		len++;
		buf++;
		hb = 1;
	}
	
	i = (num >> 16) & 0xFF;
	if(hb == 1 ) {
		*buf = (unsigned char ) i;
		len++;
		buf++;
	} else if( i > 0) {
		if(i > 127) {
			*buf = 0x00;
			buf++;
			len++;
		}
		*buf = (unsigned char ) i;
		len++;
		buf++;
		hb = 1;
	}
	
	i = (num >> 8) & 0xFF;
	if(hb == 1 ) {
		*buf = (unsigned char ) i;
		len++;
		buf++;
	} else if( i > 0) {
		if(i > 127) {
			*buf = 0x00;
			buf++;
			len++;
		}
		*buf = (unsigned char ) i;
		len++;
		buf++;
		hb = 1;
	}

	i = num & 0xFF;
	if( (i > 127) & (hb == 0) ) {
		*buf = 0x00;
		buf++;
		len++;
	}
	*buf = (unsigned char ) i;
	len++;

	return len;
}

/* ----------------------------------------------------------------------------
 * @brief	раскод-ем unsigned integer
 * @n		кол-во байт занимает число
 * @return	 [integer]
 */
int bo_uncode_int(unsigned char *buf, int n)
{
	int i = 0, val = 0;
	for(; i < n; i++) {
		val <<= 8;
		val += *(buf + i);
	}
	return val;
}
/* ----------------------------------------------------------------------------
 * @brief	код-ем OCTET STRING
 * @return	кол-во байт занимает строка	
 */
int bo_code_string(unsigned char *buf, unsigned char *str, int len)
{
	int ans = 0;
	*buf = ASN1_STRING;
	buf++;
	ans++;
	bo_code_len(buf, len);
	buf += bo_len_size(len);
	ans += bo_len_size(len);
	memcpy(buf, str, len);
	ans += len;
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	кол-во байт для хран одно числа
 */
int bo_oid_size(int num)
{
    int len = 1;
    
    while (num > 127)
    {
        num >>= 7;
        len++;
    }
    
    return len;
}

/* ----------------------------------------------------------------------------
 * @return	длина OID 
 */
int bo_oid_length(int *oid, int n)
{
    int data_len = 1;
    int i;
    
    for (i = 2; i < n; i++) {
        data_len += bo_oid_size( *(oid + i) );
    }
    
    return data_len;
}

/* ----------------------------------------------------------------------------
 * @return      возвр кол-во байт, которые занял oid в buf 
 */
int bo_code_oid(int oid, unsigned char *buf)
{
        int ans = 1, i = 0, len = 0, v;
        
	if(oid < 128) {
		*buf = oid;
		goto exit;
	}
	
	len = bo_oid_size(oid);
	ans = len;
	for (i = len - 1; i >= 0; i--) {
		v = oid & 0x7F;
		if (i != len - 1) v |= 0x80;
		oid >>= 7;
		*(buf + i) = (char) v;
	}
	exit:
        return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	parse oid
 * @val		oid
 * @return	размер oid
 */
int bo_uncode_oid(unsigned char *buf, unsigned int *val)
{
	int len = 1; unsigned int t = 0, i = 0;
	unsigned char *temp = NULL;
	/* ДЛИНА OID */
	temp = buf;
	while( *temp > 127) {
		len++;
		temp = temp + 1; 
	}
	
	for(i = 0; i < len; i++) {
		temp = buf + i;
		t = (t<<7) + (*temp & 0x7F);
	}
	
	*val = t;
	return len;
}

/* 0x42 */