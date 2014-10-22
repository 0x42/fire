#include "unity_fixture.h"
#include "../../src/tools/dbgout.h"

TEST_GROUP    (route);

TEST_SETUP    (route) {};
TEST_TEAR_DOWN(route) {};

TEST(route, simpleTest)
{
	gen_tbl_crc16modbus();
	printf("simpleTest() ... \n");
	int ans = 1;
	char *msg = "110:192.168.100.100:2"; //21
	unsigned int n = 10;
	int crc = crc16modbus(msg, n);
	unsigned char buf1[2] = {0};
	boIntToChar(crc, buf1);
	
	unsigned char buf2[2] = {0};
	boIntToChar(n, buf2);
	printf("%02x %02x\n", buf2[0], buf2[1]);
	
//	printf("len+msg+crc[%02x|%s|%02x]\n", buf2, msg, buf1);
	TEST_ASSERT_EQUAL(1, ans);
}