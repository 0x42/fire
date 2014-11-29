#ifndef BO_ASN_H
#define	BO_ASN_H
#include <stdlib.h>
#include <stdio.h>
/* ----------------------------------------------------------------------------
 * BER (BASIC ENCODING RULE) http://en.wikipedia.org/wiki/X.690
 * SEQUENCE = 0x10 -> 6byte set 1(constructed type) -> 0x30
 * INTEGER  = 0x02 -> 6byte set 0(primitive type)   -> 0x02
 */
/* identifier */
enum {
    ASN1_INTEGER  = 0x02,
    ASN1_SEQUENCE = 0x30
};
/* -----------------------------------------------------------------------------
 * @brief   вычисл размер буфера для хранения длины примитивов ASN
 */
int bo_len_size (int len);

/* ----------------------------------------------------------------------------
 * @brief	кодируем len согласно BER(http://en.wikipedia.org/wiki/X.690)
 * @len_ber	массив куда будет записан рез-тат
 */
void bo_code_len(char *len_ber, int len);

/* ----------------------------------------------------------------------------
 * @return	кол-во байт из которых состоит поле BER(Length)
 */
int bo_len_ber_size (char *len_ber);

/* ----------------------------------------------------------------------------
 * @brief	читаем длину из закод поля
 * @len_ber	поле Length закод-ое по правилу BER		
 */
unsigned int bo_uncode_len(char *len_ber);


#endif	/* BO_ASN_H */
/* 0x42 */