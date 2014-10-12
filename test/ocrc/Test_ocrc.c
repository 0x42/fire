
#include "ocrc.h"

#include "unity_fixture.h"


TEST_GROUP(ocrc);

char *str = "123456789";
unsigned short check = 0x4B37;
/*
unsigned short width = 16;
unsigned short poly = 0x8005;
unsigned short reg_init = 0xFFFF;
unsigned short xorout = 0x0000;
int refin = 1;
int refout = 1;
*/

TEST_SETUP(ocrc)
{
	gen_tbl_crc16modbus();
}

TEST_TEAR_DOWN(ocrc)
{
}


TEST(ocrc, GetCRC_123456789)
{
	TEST_ASSERT_EQUAL(check, crc16modbus(str, 9));
}

