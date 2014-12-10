#ifndef BO_ASN_H
#define	BO_ASN_H
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
/* ----------------------------------------------------------------------------
 * BER (BASIC ENCODING RULE) http://en.wikipedia.org/wiki/X.690
 * SEQUENCE = 0x10 -> 6byte set 1(constructed type) -> 0x30
 * INTEGER  = 0x02 -> 6byte set 0(primitive type)   -> 0x02
 */
/* identifier */
enum {
    ASN1_INTEGER  = 0x02,
    ASN1_STRING   = 0x04,
    ASN1_SEQUENCE = 0x30,
    OID_TYPE      = 0x06,
    GET_REQ_PDU   = 0xA0
};
/* -----------------------------------------------------------------------------
 * @brief   вычисл размер буфера для хранения длины примитивов ASN
 */
int bo_len_size (int len);

/* ----------------------------------------------------------------------------
 * @brief	кодируем len согласно BER
 * @len_ber	массив куда будет записан рез-тат
 */
void bo_code_len(unsigned char *len_ber, int len);

/* ----------------------------------------------------------------------------
 * @return	кол-во байт из которых состоит поле BER(Length)
 */
int bo_len_ber_size (unsigned char *len_ber);

/* ----------------------------------------------------------------------------
 * @brief	читаем длину из закод поля
 * @len_ber	поле Length закод-ое по правилу BER		
 */
unsigned int bo_uncode_len(unsigned char *len_ber);

/* ----------------------------------------------------------------------------
 * @brief	кол-во байт необх-мые для хранения INTEGER
 */
int bo_int_size(int num);

/* ----------------------------------------------------------------------------
 * @brief	кодируем INTEGER(раск-ем по основанию 256)
 * @return	размер buf	
 */
int bo_code_int(unsigned char *buf, int num);

/* ----------------------------------------------------------------------------
 * @return	[-1] error [INT]
 */
int bo_uncode_int(unsigned char *buf);

/* ----------------------------------------------------------------------------
 * @brief	код-ем OCTET STRING	
 */
int bo_code_string(unsigned char *buf, unsigned char *str, int len);

int bo_oid_length(int *oid, int n);

int bo_oid_size(int num);
/* ----------------------------------------------------------------------------
 * @return      возвр колво байт, которые занял oid в buf 
 */
int bo_code_oid(int oid, unsigned char *buf);

#endif	/* BO_ASN_H */
/* 0x42 */