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
void bo_code_len(char *len_ber, int len) 
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
int bo_len_ber_size (char *len_ber)
{
	int n = 0, ans = 0;
	n = *(unsigned char *)len_ber;
	
	if(n < 128) ans = 1;
	else {
		ans = 2;
		/* удаляем старший бит согласно BER */
		n = n ^ 0x80;
		ans = (unsigned int)n;
	}
	return ans;
}

/* ----------------------------------------------------------------------------
 * @brief	читаем длину из закод поля
 * @len_ber	поле Length закод-ое по правилу BER		
 */
unsigned int bo_uncode_len(char *len_ber)
{
	unsigned int len = 0, i = 0, size = 0;
	unsigned char buf;
	
	size = bo_len_ber_size(len_ber);
	
	if(size == 1) {
		len = *(unsigned char *)len_ber;
	} else {
		for(i = 1; i != size + 1; i++) {
			if( i != 1) len <<= 8;
			buf = (unsigned char)*(len_ber + i);
			len = len |  (unsigned int)buf;
		}
	}
	
	return len;
}

/* 0x42 */