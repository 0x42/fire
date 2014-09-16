
#include "ocfg.h"

#include "unity_fixture.h"


TEST_GROUP(ocfg);


TOHT *ht;

char cfgname[] = "t.cfg";

char line [513];
char sect [513];
int lno;


void cfg_dump(TOHT *ht, FILE *out)
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


TEST_SETUP(ocfg)
{
	lno = 0;
	memset(line, 0, 512);
	memset(sect, 0, 512);
	
	ht = ht_new(0);
}

TEST_TEAR_DOWN(ocfg)
{
	cfg_dump(ht, stderr);
	ht_free(ht);
}


TEST(ocfg, CfgLoad)
{	
	TEST_ASSERT_NOT_NULL(ht = cfg_load("test.cfg"));
}

TEST(ocfg, CfgParseLine)
{
	sprintf(line, "[section1]");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	sprintf(line, "key1=value1");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	sprintf(line, "key2 = value2");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	sprintf(line, "key3 = value3#commentary");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
}

TEST(ocfg, CfgGetString)
{
	sprintf(line, "[section2]");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	sprintf(line, "key1=value1");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	sprintf(line, "key2 = value2 #commentary");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));

	sprintf(line, "[section21]");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	sprintf(line, "key21=value21");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	TEST_ASSERT_EQUAL_STRING(
		"value2", cfg_getstring(ht, "section2:key2", NULL));

	TEST_ASSERT_NULL(cfg_getstring(ht, "section2:key3", NULL));

	TEST_ASSERT_EQUAL_STRING(
		"value21", cfg_getstring(ht, "section21:key21", NULL));
}

TEST(ocfg, CfgGetStringDefvalue)
{
	TEST_ASSERT_NULL(cfg_getstring(ht, "section1:key2", NULL));
}


TEST(ocfg, CfgGetInt)
{
	sprintf(line, "[section2]");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	sprintf(line, "key1=1");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	sprintf(line, "key2 = 2 #commentary");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));

	sprintf(line, "[section21]");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	sprintf(line, "key21=21");
	TEST_ASSERT_EQUAL(0, cfg_parse_line(ht, cfgname, line, lno++, sect));
	
	TEST_ASSERT_EQUAL(2, cfg_getint(ht, "section2:key2", -1));

	TEST_ASSERT_EQUAL(-1, cfg_getint(ht, "section2:key3", -1));

	TEST_ASSERT_EQUAL(21, cfg_getint(ht, "section21:key21", -1));
}

TEST(ocfg, CfgGetIntDefvalue)
{
	TEST_ASSERT_EQUAL(-1, cfg_getint(ht, "section1:key2", -1));
}


TEST(ocfg, CfgCreateFile)
{
	FILE *out;

	out = fopen("moxa.cfg", "w");
	fprintf(out,
		"#\n"
		"# This file created by cfg_save().\n"
		"# Do not modify it!\n"
		"#\n"	  
		"\n");

	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_eth:ip", "192.168.100.127"));
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_eth:gw", "192.168.100.1"));

	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_rs:pc1", "11"));
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_rs:pc4", "14"));
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_rs:ac1", "128"));
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_rs:ac2", "129"));
	
	cfg_remove(ht, "node1_rs:pc4");
	cfg_remove(ht, "node1_eth:gw");
			  
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_eth:mask", "255.255.255.0"));
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_eth:gw", "192.168.100.2"));
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_rs:pc2", "12"));
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_rs:pc3", "13"));
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_rs:ac3", "130"));
	TEST_ASSERT_EQUAL(0, cfg_put(ht, "node1_rs:ac4", "131"));
	
	cfg_save(ht, out);
	fclose(out);
}


