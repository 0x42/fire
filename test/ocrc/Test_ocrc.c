
#include "ocrc.h"

#include "unity_fixture.h"


TEST_GROUP(ocrc);

char *str = "123456789";
unsigned short check = 0x4B37;

char *str2 = "\x05\x82\xE6\xC2\x01\x08\x30\x30\x30\x30\x30\x37\x33\x38";
unsigned short check2 = 0xC0C0;

char *str3 = "\x01\x81\xD4\x5D\0x00\0x00";
unsigned short check3 = 0xC0C0;

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

TEST(ocrc, GetCRC_738)
{
	TEST_ASSERT_EQUAL(check2, crc16modbus(str2, 14));
}

TEST(ocrc, GetCRC_d45d)
{
	TEST_ASSERT_EQUAL(check3, crc16modbus(str3, 6));
}
