#include "unity_fixture.h"

TEST_GROUP_RUNNER(fifo)
{
	/* NEED RUN SERVER */
//	RUN_TEST_CASE(fifo, sendOneByte);
	
//	RUN_TEST_CASE(fifo, getFromEmptyFifo);
//	RUN_TEST_CASE(fifo, sendSETL10MSG10);
//	RUN_TEST_CASE(fifo, sendSETL10MSG9);
//	RUN_TEST_CASE(fifo, sendOnlyHead);
//	RUN_TEST_CASE(fifo, send100MSGSET10);
	RUN_TEST_CASE(fifo, testThrModel);
//	RUN_TEST_CASE(fifo, testIDmsg);
	/* тест FIFO */
//	RUN_TEST_CASE(fifo, addOneGetOne);
//	RUN_TEST_CASE(fifo, fifo2add3);
//	RUN_TEST_CASE(fifo, add100get100);
//	RUN_TEST_CASE(fifo, addget100);
//	RUN_TEST_CASE(fifo, getFromEmptyFIFO);
//	RUN_TEST_CASE(fifo, getToLittleBuf);
//	RUN_TEST_CASE(fifo, setNullMsg);
//	RUN_TEST_CASE(fifo, addBigMsgThanItemFifo);
//	RUN_TEST_CASE(fifo, fifoAddDel);
	/* TEST FIFO OUT*/
//	RUN_TEST_CASE(fifo, out_addOneGetOne);
//	RUN_TEST_CASE(fifo, out_addToOneIP);
//	RUN_TEST_CASE(fifo, out_overflowFifo);
}
