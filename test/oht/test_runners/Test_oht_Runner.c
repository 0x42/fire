
#include "unity_fixture.h"

TEST_GROUP_RUNNER(oht)
{
	RUN_TEST_CASE(oht, CreateHashTable);
	RUN_TEST_CASE(oht, CreateHashTableLength);
	RUN_TEST_CASE(oht, HashTablePutStringValue);
	RUN_TEST_CASE(oht, HashTablePutEmptyStringValue);
	RUN_TEST_CASE(oht, HashTablePutNullValue);
	RUN_TEST_CASE(oht, HashTablePutEmptyStringKey);
	RUN_TEST_CASE(oht, HashTablePutNullKey);
	RUN_TEST_CASE(oht, HashTableGetString);
	RUN_TEST_CASE(oht, HashTableGetNull);
	RUN_TEST_CASE(oht, HashTableGetDefault);
	RUN_TEST_CASE(oht, HashTableModifyValue);
	RUN_TEST_CASE(oht, HashTableRemoveKey);
	RUN_TEST_CASE(oht, HashTableRandom);
}

