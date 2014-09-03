
#include "oht.h"

#include "unity_fixture.h"


TEST_GROUP(oht);


TOHT *ht;
FILE *cfg;

static char ckey[90];
static char cval[90];


TEST_SETUP(oht)
{
	sprintf(ckey, "%04d", 1);
	sprintf(cval, "string-%04d", 1);
}

TEST_TEAR_DOWN(oht)
{
}

void ht_dump(TOHT *ht, FILE *out)
{
	int i;

	if (ht == NULL || out == NULL) return;
	
	if (ht->n < 1) {
		fprintf(out, "empty table\n");
		return;
	}
	for (i=0; i<ht->size; i++) {
		if (ht->key[i])
			fprintf(out, "%20d\t%20s\t[%s]\n",
				ht->hash[i],
				ht->key[i],
				ht->val[i] ? ht->val[i] : "UNDEF");
	}
	return;
}


TEST(oht, CreateHashTable)
{
	TEST_ASSERT_NOT_NULL(ht_new(0));
}

TEST(oht, CreateHashTableLength)
{
	int HT_MINSIZE = 8;

	TEST_ASSERT_EQUAL(HT_MINSIZE, ht_new(0)->size);
	TEST_ASSERT_EQUAL(12, ht_new(12)->size);
}

TEST(oht, HashTablePutStringValue)
{	
	ht = ht_new(0);
	
	TEST_ASSERT_EQUAL(0, ht_put(ht, ckey, cval));
	
	cfg = fopen("dump-string.txt", "w");
	ht_dump(ht, cfg);
	fclose(cfg);
}

TEST(oht, HashTablePutEmptyStringValue)
{	
	ht = ht_new(0);
	
	TEST_ASSERT_EQUAL(0, ht_put(ht, ckey, ""));
	
	cfg = fopen("dump-empty-string-value.txt", "w");
	ht_dump(ht, cfg);
	fclose(cfg);
}

TEST(oht, HashTablePutNullValue)
{	
	ht = ht_new(0);
	
	TEST_ASSERT_EQUAL(0, ht_put(ht, ckey, NULL));
	
	cfg = fopen("dump-null-value.txt", "w");
	ht_dump(ht, cfg);
	fclose(cfg);
}

TEST(oht, HashTablePutEmptyStringKey)
{	
	ht = ht_new(0);
	
	TEST_ASSERT_EQUAL(0, ht_put(ht, "", NULL));
	
	cfg = fopen("dump-empty-string-key.txt", "w");
	ht_dump(ht, cfg);
	fclose(cfg);
}

TEST(oht, HashTablePutNullKey)
{	
	ht = ht_new(0);
	
	TEST_ASSERT_EQUAL(-1, ht_put(ht, NULL, NULL));
	
	cfg = fopen("dump-null-key.txt", "w");
	ht_dump(ht, cfg);
	fclose(cfg);
}

TEST(oht, HashTableGetString)
{	
	ht = ht_new(0);
	ht_put(ht, ckey, cval);
	
	TEST_ASSERT_EQUAL_STRING(cval, ht_get(ht, ckey, "defvalue"));
}

TEST(oht, HashTableGetNull)
{	
	ht = ht_new(0);
	ht_put(ht, ckey, NULL);
	
	TEST_ASSERT_NULL(ht_get(ht, ckey, "defvalue"));
}

TEST(oht, HashTableGetDefault)
{	
	ht = ht_new(0);
	ht_put(ht, ckey, cval);
	
	TEST_ASSERT_EQUAL_STRING("defvalue", ht_get(ht, "100", "defvalue"));
}

TEST(oht, HashTableModifyValue)
{	
	ht = ht_new(0);
	ht_put(ht, ckey, cval);	
	TEST_ASSERT_EQUAL_STRING(cval, ht_get(ht, ckey, "defvalue"));

	ht_put(ht, ckey, "new value");
	TEST_ASSERT_EQUAL_STRING("new value", ht_get(ht, ckey, "defvalue"));
}

TEST(oht, HashTableRemoveKey)
{	
	ht = ht_new(0);
	ht_put(ht, ckey, cval);	
	TEST_ASSERT_EQUAL_STRING(cval, ht_get(ht, ckey, "defvalue"));

	ht_remove(ht, ckey);
	TEST_ASSERT_EQUAL_STRING("defvalue", ht_get(ht, ckey, "defvalue"));
}

TEST(oht, HashTableRandom)
{
	int i, j;
	
	ht = ht_new(0);
	for (i=0; i<12; i++) {
		sprintf(ckey, "%04d", i);
		sprintf(cval, "value-%04d", i);
		ht_put(ht, ckey, cval);
	}
	
	cfg = fopen("dump-rand-init-put.txt", "w");
	ht_dump(ht, cfg);
	fclose(cfg);
	
	for (j=6; j<10; j++) {
		sprintf(ckey, "%04d", j);
		ht_remove(ht, ckey);
	}

	cfg = fopen("dump-rand-remove.txt", "w");
	ht_dump(ht, cfg);
	fclose(cfg);
	
	for (i=8; i<11; i++) {
		sprintf(ckey, "%04d", i);
		sprintf(cval, "new-value-%04d", i);
		ht_put(ht, ckey, cval);
	}

	cfg = fopen("dump-rand-post-put.txt", "w");
	ht_dump(ht, cfg);
	fclose(cfg);
}

