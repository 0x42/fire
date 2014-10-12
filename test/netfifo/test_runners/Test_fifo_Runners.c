#include "unity_fixture.h"

TEST_GROUP_RUNNER(fifo)
{
	/*тестируем запущеный сервер 127.0.0.1:8888*/
	RUN_TEST_CASE(fifo, sendOneByte);
	RUN_TEST_CASE(fifo, sendSETL10MSG10);
	RUN_TEST_CASE(fifo, sendSETL10MSG9);
	RUN_TEST_CASE(fifo, sendSETL10MSG23);
	RUN_TEST_CASE(fifo, sendOnlyHead);
	RUN_TEST_CASE(fifo, sendSETL10240MSG10240);
//	RUN_TEST_CASE(fifo, send100MSGSET1024);
}
