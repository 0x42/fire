#include <string.h>
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
	char packet[23] = {0};
	unsigned int n = 21;
	int crc = crc16modbus(msg, n);
	unsigned char buf1[2] = {0};
	boIntToChar(crc, buf1);
	
	memcpy(packet, msg, 21);
	printf("\nmsg[%s] packet[%s]\n", msg, packet);
	printf("crc[%d]=[%02x %02x]\n", crc, buf1[0], buf1[1]);
	packet[21] = buf1[0];
	packet[22] = buf1[1];
//	
	ans = bo_sendDataFIFO("127.0.0.1", 8889, packet, sizeof(packet));
	TEST_ASSERT_EQUAL(1, ans);
}