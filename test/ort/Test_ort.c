
#include "ort.h"

#include "unity_fixture.h"


TEST_GROUP(ort);


TOHT *rt;
char line [10];


void rt_dump(TOHT *ht, FILE *out)
{
	int i;

	if (ht == NULL || out == NULL) return;
	for (i=0; i<ht->size; i++) {
		if (ht->key[i] == NULL)
			continue;
		if (ht->val[i] != NULL) {
			fprintf(out, "[%s]=[%s]\n", ht->key[i], ht->val[i]);
		} else {
			fprintf(out, "[%s]=UNDEF\n", ht->key[i]);
		}
	}
	return;
}


TEST_SETUP(ort)
{
	memset(line, 0, 10);
	
	rt = ht_new(0);
}

TEST_TEAR_DOWN(ort)
{
	rt_dump(rt, stderr);
	rt_free(rt);
}


TEST(ort, RtLoad)
{	
	TEST_ASSERT_NOT_NULL(rt = rt_load("rt_load_test"));
}


TEST(ort, RtGetPort)
{
	rt = rt_load("rt_load_test");
	
	TEST_ASSERT_EQUAL(1, rt_getport(rt, "011:144", -1));
	TEST_ASSERT_EQUAL(-1, rt_getport(rt, "007:129", -1));
	TEST_ASSERT_EQUAL(2, rt_getport(rt, "212:074", -1));
}

TEST(ort, RtGetPortDefvalue)
{
	TEST_ASSERT_EQUAL(-1, rt_getport(rt, "001:022", -1));
}


TEST(ort, RtCreateFile1)
{
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:128", "1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:129", "1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:012", "2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:013", "2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:014", "2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:015", "2"));
	
	rt_remove(rt, "007:014");
	rt_remove(rt, "007:129");
	
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:013", "1"));
	
	TEST_ASSERT_EQUAL(0, rt_put(rt, "203:134", "1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "203:024", "2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "203:025", "2"));
	
	rt_save(rt, "rt_glob1");
}

TEST(ort, RtCreateFile2)
{
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:128", "1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:129", "1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:012", "2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:013", "2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:014", "2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "007:015", "2"));
	
	TEST_ASSERT_EQUAL(0, rt_put(rt, "203:134", "1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "203:024", "2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "203:025", "2"));
	
	rt_save(rt, "rt_glob2");
}


