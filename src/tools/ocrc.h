/*
 *		Вычисление контрольной суммы CRC16-Modbus
 *
 * Version:	@(#)ocrc.h	1.0.0	01/09/14
 * Authors:	ovelb
 *
 */

#ifndef _OCRC_H
#define _OCRC_H

/* #include <stdlib.h> */


#define TBL_IDX_WIDTH 8
#define TBL_LENGTH (1<<TBL_IDX_WIDTH)

/*
void gen_tbl_crc16modbus(unsigned short poly, unsigned short w, int refin);
unsigned int crc16modbus(char *str,
			 int len,
			 unsigned short reg_init,
			 unsigned short w,
			 int refin,
			 int refout,
			 unsigned short xorout);
*/
void gen_tbl_crc16modbus();
unsigned int crc16modbus(char *str, int len);


#endif	/* _OCRC_H */

