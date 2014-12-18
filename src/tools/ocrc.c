/*
 *		Вычисление контрольной суммы CRC16-Modbus
 *
 * Version:	@(#)ocrc.c	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#include "ocrc.h"

unsigned short width = 16;       /** Длина регистра в битах. */
unsigned short poly = 0x8005;    /** Представление полинома x^16+x^15+x^2+1. */
unsigned short init = 0xFFFF;    /** Начальное значение регистра. */
unsigned short xorout = 0x0000;  /** Значение применяемое к результату
				  * оператором XOR. */
int refin = 1;   /** Флаг для применения функции reflect(). */
int refout = 1;  /** Флаг для применения функции reflect() на выходе. */


static unsigned short TBL_CRC16[TBL_LENGTH];

/**
 * reflect - Инверсия порядка следования битов в регистре.
 * @reg: Регистр для инверсии.
 * @w:   Длина регистра в битах.
 * @return   Значение преобразованного регистра.
 *
 * Процедура должна использоваться при условии Refin=true или Refout=true.
 * Пример: регистр на входе - 0x10010001, после преобразования - 0x10001001.
 */
static unsigned short reflect(unsigned short reg, unsigned short w)
{
        unsigned short x = reg & 0x01;
	int i;
	
        for (i=0; i<(w-1); i++) {
		reg >>= 1;
		x = (x << 1) | (reg & 0x01);
	}
	
        return x;
}

/**
 * gen_tbl_crc16modbus - Генерация таблицы для вычисления контрольной суммы
 *                       CRC16-Modbus.
 * @poly:  Представление полинома x^16+x^15+x^2+1.
 * @width: Длина регистра в битах.
 * @refin: Флаг для применения функции reflect().
 *
 * Создает таблицу для вычисления контрольной суммы CRC16-Modbus на
 * основе данных параметров.
 */
void gen_tbl_crc16modbus()
{
	unsigned short msb_mask = 0x1 << (width - 1);
	unsigned short mask = ((msb_mask - 1) << 1) | 1;
	unsigned short reg, i, j;
	
	for (i=0; i<TBL_LENGTH; i++) {
		reg = i;
		if (refin)
			reg = reflect(reg, TBL_IDX_WIDTH);
		reg = reg << (width - TBL_IDX_WIDTH);
		for (j=0; j<TBL_IDX_WIDTH; j++) {
			if ((reg & msb_mask) != 0)
				reg = (reg << 1) ^ poly;
			else
				reg = (reg << 1);
		}
		if (refin)
			reg = reflect(reg, width);
		TBL_CRC16[i] = reg & mask;
	}
}

/**
 * crc16modbus - Вычисление контрольной суммы CRC16-Modbus входной строки.
 * @str:    Входная строка.
 * @len:    Длина строки в байтах.
 * @init:   Начальное значение регистра.
 * @width:  Длина регистра в битах.
 * @refin:  Флаг для применения функции reflect().
 * @refout: Флаг для применения функции reflect() на выходе.
 * @xorout: Значение применяемое к результату оператором XOR.
 * @return    Значение контрольной суммы.
 *
 * Вычисляет контрольную сумму CRC16-Modbus входной строки на
 * основе данных параметров.
 */

unsigned int crc16modbus(char *str, int len)
{
	unsigned short reg = init;
	unsigned short msb_mask = 0x1 << (width - 1);
	unsigned short mask = ((msb_mask - 1) << 1) | 1;
	unsigned short idx;
	
	if (refin) {
		reg = reflect(reg, width);
		while (len--) {
			idx = (reg ^ *str++) & 0xff;
			reg = ((reg >> TBL_IDX_WIDTH) ^ TBL_CRC16[idx]) & mask;
		}
		reg = reflect(reg, width) & mask;
	} else {
		while (len--) {
			idx = ((reg >> (width - TBL_IDX_WIDTH)) ^ *str++) & 0xff;
			reg = ((reg << TBL_IDX_WIDTH) ^ TBL_CRC16[idx]) & mask;
		}
	}
	if (refout) reg = reflect(reg, width);

	return (reg ^ xorout);
}

