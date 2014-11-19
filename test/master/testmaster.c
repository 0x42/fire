#include <stdlib.h>
#include <string.h>
#include "../../src/tools/dbgout.h"
#include "../../src/nettcp/bo_net.h"
#include "unity_fixture.h"

TEST_GROUP(master);

TEST_SETUP(master) {}

TEST_TEAR_DOWN(master) {}

TEST(master, simpleTest)
{
	gen_tbl_crc16modbus();
	printf("master -> simpleTest() ... \n");
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
	ans = bo_sendDataFIFO("127.0.0.1", 8890, packet, sizeof(packet));
	if(ans == -1) {
		printf("can't send value");
	}
	TEST_ASSERT_EQUAL(1, ans);
}

TEST(master, setConnectTest)
{
	int ans = -1;
	char *ip = "192.168.1.128";
	printf("master->setConnectTest ... \n");
	int in = bo_setConnect(ip, 8890);
	if(in > 0) ans = 1;
	else printf("can't connect to %s", ip);
	TEST_ASSERT_EQUAL(1, ans);
}



