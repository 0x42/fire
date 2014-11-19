
#include "ocs.h"

#include "unity_fixture.h"


TEST_GROUP(ocs);


TEST_SETUP(ocs)
{
	gen_tbl_crc16modbus();
}

TEST_TEAR_DOWN(ocs)
{
}


TEST(ocs, CsReaderSt1_FFC0)
{
	struct thr_rx_buf b;
	int fl = 0;
	
	fl = read_byte(&b, '\xFF', fl);
	TEST_ASSERT_EQUAL(0, fl);

	fl = read_byte(&b, '\xC0', fl);
	TEST_ASSERT_EQUAL(1, fl);
}

TEST(ocs, CsReaderSt1_DC)
{
	struct thr_rx_buf b;
	
	TEST_ASSERT_EQUAL(1, read_byte(&b, '\xDC', 2));
}

TEST(ocs, CsReaderSt1_DD)
{
	struct thr_rx_buf b;
	
	TEST_ASSERT_EQUAL(1, read_byte(&b, '\xDD', 2));
}

TEST(ocs, CsReaderSt2_DB)
{
	struct thr_rx_buf b;
	
	TEST_ASSERT_EQUAL(2, read_byte(&b, '\xDB', 1));
}

TEST(ocs, CsReaderSt4_D0)
{
	struct thr_rx_buf b;
	
	TEST_ASSERT_EQUAL(4, read_byte(&b, '\xD0', 2));
}

TEST(ocs, CsReaderSt3_000)
{
	struct thr_rx_buf b;	
	int fl = 0;
	
	b.wpos = 0;

	fl = read_byte(&b, '\xFF', fl);
	fl = read_byte(&b, '\xC0', fl);
	fl = read_byte(&b, '0', fl);
	fl = read_byte(&b, '0', fl);
	fl = read_byte(&b, '0', fl);
	fl = read_byte(&b, '\x65', fl);
	fl = read_byte(&b, '\xDB', fl);
	fl = read_byte(&b, '\xDD', fl);
	fl = read_byte(&b, '\xC0', fl);

	TEST_ASSERT_EQUAL(3, fl);
}

TEST(ocs, CsReaderSt4_000)
{
	struct thr_rx_buf b;	
	int fl = 0;
	
	b.wpos = 0;

	fl = read_byte(&b, '\xFF', fl);
	fl = read_byte(&b, '\xC0', fl);
	fl = read_byte(&b, '0', fl);
	fl = read_byte(&b, '0', fl);
	fl = read_byte(&b, '0', fl);
	fl = read_byte(&b, '\x65', fl);
	fl = read_byte(&b, '\xDC', fl);
	fl = read_byte(&b, '\xDD', fl);
	fl = read_byte(&b, '\xC0', fl);

	TEST_ASSERT_EQUAL(4, fl);
}


TEST(ocs, CsWriter_123456789)
{
	struct thr_tx_buf b;
	unsigned int crc;
	int i;
	int n;  /** Кол-во байт подготовленных для передачи. */
	char buf[1032] = {0};
	char *str = "123456789";
	char *check = "0xFF 0xC0 0x31 0x32 0x33 0x34 0x35 0x36 0x37 0x38 0x39 0x37 0x4B 0xC0";

	b.wpos = 0;
	put_txBuf(&b, '1');
	put_txBuf(&b, '2');
	put_txBuf(&b, '3');
	put_txBuf(&b, '4');
	put_txBuf(&b, '5');
	put_txBuf(&b, '6');
	put_txBuf(&b, '7');
	put_txBuf(&b, '8');
	put_txBuf(&b, '9');
	
	crc = crc16modbus(b.buf, 9);

	put_txBuf(&b, (char)(crc & 0xff));
	put_txBuf(&b, (char)((crc >> 8) & 0xff));

	n = prepare_buf_tx(&b, buf);
	
	TEST_ASSERT_EQUAL(14, n);

	printf("\n    Кадр: %s", str);
	printf("\n Ожидаем: %s", check);
	printf("\nПолучили: ");
	for (i=0; i<n; i++)
		printf("0x%02X ", (unsigned char)buf[i]);
	printf("\n");
}

TEST(ocs, CsWriter_738)
{
	struct thr_tx_buf b;
	unsigned int crc;
	int i;
	int n;  /** Кол-во байт подготовленных для передачи. */
	char buf[1032] = {0};
	char *str = "0x05 0x82 0xE6 0xC2 0x01 0x08 0x30 0x30 0x30 0x30 0x30 0x37 0x33 0x38";
	char *check = "0xFF 0xC0 0x05 0x82 0xE6 0xC2 0x01 0x08 0x30 0x30 0x30 0x30 0x30 0x37 0x33 0x38 0xDB 0xDC 0xDB 0xDC 0xC0";

	b.wpos = 0;
	put_txBuf(&b, '\x05');
	put_txBuf(&b, '\x82');
	put_txBuf(&b, '\xE6');
	put_txBuf(&b, '\xC2');
	put_txBuf(&b, '\x01');
	put_txBuf(&b, '\x08');
	put_txBuf(&b, '\x30');
	put_txBuf(&b, '\x30');
	put_txBuf(&b, '\x30');
	put_txBuf(&b, '\x30');
	put_txBuf(&b, '\x30');
	put_txBuf(&b, '\x37');
	put_txBuf(&b, '\x33');
	put_txBuf(&b, '\x38');
	
	crc = crc16modbus(b.buf, 14);

	put_txBuf(&b, (char)(crc & 0xff));
	put_txBuf(&b, (char)((crc >> 8) & 0xff));

	n = prepare_buf_tx(&b, buf);
	
	TEST_ASSERT_EQUAL(21, n);

	printf("\n    Кадр: %s", str);
	printf("\n Ожидаем: %s", check);
	printf("\nПолучили: ");
	for (i=0; i<n; i++)
		printf("0x%02X ", (unsigned char)buf[i]);
	printf("\n");
}

TEST(ocs, CsWriter_C0DBC0)
{
	struct thr_tx_buf b;
	unsigned int crc;
	int i;
	int n;  /** Кол-во байт подготовленных для передачи. */
	char buf[1032] = {0};
	char *str = "0xC0 0xDB 0xC0";
	char *check = "0xFF 0xC0 0xDB 0xDC 0xDB 0xDD 0xDB 0xDC 0x2B 0x5C 0xC0";

	b.wpos = 0;
	put_txBuf(&b, '\xC0');
	put_txBuf(&b, '\xDB');
	put_txBuf(&b, '\xC0');
	
	crc = crc16modbus(b.buf, 3);

	put_txBuf(&b, (char)(crc & 0xff));
	put_txBuf(&b, (char)((crc >> 8) & 0xff));

	n = prepare_buf_tx(&b, buf);
	
	TEST_ASSERT_EQUAL(11, n);

	printf("\n    Кадр: %s", str);
	printf("\n Ожидаем: %s", check);
	printf("\nПолучили: ");
	for (i=0; i<n; i++)
		printf("0x%02X ", (unsigned char)buf[i]);
	printf("\n");
}

TEST(ocs, CsWriter_FFFFF)
{
	struct thr_tx_buf b;
	unsigned int crc;
	int i;
	int n;  /** Кол-во байт подготовленных для передачи. */
	char buf[1032] = {0};
	char *str = "FFFFF";
	char *check = "0xFF 0xC0 0x46 0x46 0x46 0x46 0x46 0xEA 0xDB 0xDC 0xC0";

	b.wpos = 0;
	put_txBuf(&b, 'F');
	put_txBuf(&b, 'F');
	put_txBuf(&b, 'F');
	put_txBuf(&b, 'F');
	put_txBuf(&b, 'F');
	
	crc = crc16modbus(b.buf, 5);

	put_txBuf(&b, (char)(crc & 0xff));
	put_txBuf(&b, (char)((crc >> 8) & 0xff));

	n = prepare_buf_tx(&b, buf);
	
	TEST_ASSERT_EQUAL(11, n);

	printf("\n    Кадр: %s", str);
	printf("\n Ожидаем: %s", check);
	printf("\nПолучили: ");
	for (i=0; i<n; i++)
		printf("0x%02X ", (unsigned char)buf[i]);
	printf("\n");
}

TEST(ocs, CsWriter_000)
{
	struct thr_tx_buf b;
	unsigned int crc;
	int i;
	int n;  /** Кол-во байт подготовленных для передачи. */
	char buf[1032] = {0};
	char *str = "000";
	char *check = "0xFF 0xC0 0x30 0x30 0x30 0x65 0xDB 0xDD 0xC0";

	b.wpos = 0;
	put_txBuf(&b, '0');
	put_txBuf(&b, '0');
	put_txBuf(&b, '0');
	
	crc = crc16modbus(b.buf, 3);

	put_txBuf(&b, (char)(crc & 0xff));
	put_txBuf(&b, (char)((crc >> 8) & 0xff));

	n = prepare_buf_tx(&b, buf);
	
	TEST_ASSERT_EQUAL(9, n);

	printf("\n    Кадр: %s", str);
	printf("\n Ожидаем: %s", check);
	printf("\nПолучили: ");
	for (i=0; i<n; i++)
		printf("0x%02X ", (unsigned char)buf[i]);
	printf("\n");
}

/*
Передаваемый кадр                 Поток байт на шине RS485
“000”                <->   “\xFF\xC0000\x65\xDC\xDD\xC0”
                                             |
                                             должно быть xDB ???
*/

