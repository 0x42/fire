
#include "ort.h"

#include "unity_fixture.h"


TEST_GROUP(ort);


TOHT *rt;
char line [10];
char ip [16];


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
	
	TEST_ASSERT_EQUAL(1, rt_getport(rt, "144"));
	TEST_ASSERT_EQUAL(0, rt_getport(rt, "129"));
	TEST_ASSERT_EQUAL(2, rt_getport(rt, "074"));
}

TEST(ort, RtGetIP)
{
	rt = rt_load("rt_load_test");
	
	rt_getip(rt, "144", ip);
	TEST_ASSERT_EQUAL_STRING("192.168.1.011", ip);
	
	rt_getip(rt, "129", ip);
	TEST_ASSERT_EQUAL_STRING("NULL", ip);

	rt_getip(rt, "074", ip);
	TEST_ASSERT_EQUAL_STRING("192.168.1.212", ip);
}


TEST(ort, RtCreateFile1)
{
	TEST_ASSERT_EQUAL(0, rt_put(rt, "128", "192.168.1.007:1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "129", "192.168.1.203:1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "012", "192.168.1.007:2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "013", "192.168.1.007:2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "014", "192.168.1.203:2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "015", "192.168.1.203:2"));
	
	rt_remove(rt, "014");
	rt_remove(rt, "129");
	
	TEST_ASSERT_EQUAL(0, rt_put(rt, "013", "192.168.1.007:1"));
	
	TEST_ASSERT_EQUAL(0, rt_put(rt, "134", "192.168.1.203:1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "024", "192.168.1.007:2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "025", "192.168.1.203:2"));
	
	rt_save(rt, "rt_glob1");
}

TEST(ort, RtCreateFile2)
{
	TEST_ASSERT_EQUAL(0, rt_put(rt, "128", "192.168.1.007:1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "129", "192.168.1.007:1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "012", "192.168.1.007:2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "013", "192.168.1.007:2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "014", "192.168.1.007:2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "015", "192.168.1.007:2"));
	
	TEST_ASSERT_EQUAL(0, rt_put(rt, "134", "192.168.1.203:1"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "024", "192.168.1.203:2"));
	TEST_ASSERT_EQUAL(0, rt_put(rt, "025", "192.168.1.203:2"));
	
	rt_save(rt, "rt_glob2");
}


