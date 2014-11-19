
#include "ofnv1a.h"

#include "unity_fixture.h"


TEST_GROUP(ofnv1a);

TEST_SETUP(ofnv1a)
{
}

TEST_TEAR_DOWN(ofnv1a)
{
}


TEST(ofnv1a, GetHashIdLS)
{
	TEST_ASSERT_EQUAL(0x5D, mfnv1a("All"));
	TEST_ASSERT_EQUAL(0xC2, mfnv1a("General"));
	TEST_ASSERT_EQUAL(0x0D, mfnv1a("Vertical"));
	TEST_ASSERT_EQUAL(0x85, mfnv1a("Horizontal"));
	TEST_ASSERT_EQUAL(0x98, mfnv1a("Nozzle"));
	TEST_ASSERT_EQUAL(0xC8, mfnv1a("Detector"));
	TEST_ASSERT_EQUAL(0xA2, mfnv1a("Deployer"));
	TEST_ASSERT_EQUAL(0xC4, mfnv1a("PrimaryValve"));
	TEST_ASSERT_EQUAL(0x25, mfnv1a("SecondaryValve"));
	TEST_ASSERT_EQUAL(0x45, mfnv1a("Control"));
}

TEST(ofnv1a, GetHashQuLS)
{
	TEST_ASSERT_EQUAL(0x4D, mfnv1a("Move"));
	TEST_ASSERT_EQUAL(0x46, mfnv1a("Stop"));
	TEST_ASSERT_EQUAL(0xD4, mfnv1a("GetStatus"));
	TEST_ASSERT_EQUAL(0x9D, mfnv1a("GetParam"));
	TEST_ASSERT_EQUAL(0xF1, mfnv1a("SetParam"));
	TEST_ASSERT_EQUAL(0xD9, mfnv1a("SwitchLimits"));
	TEST_ASSERT_EQUAL(0x7E, mfnv1a("Deploy"));
	TEST_ASSERT_EQUAL(0x8D, mfnv1a("Wrap"));
	TEST_ASSERT_EQUAL(0xE5, mfnv1a("SetupQuench"));
	TEST_ASSERT_EQUAL(0xC5, mfnv1a("StartQuench"));
	TEST_ASSERT_EQUAL(0xED, mfnv1a("RetrieveQuench"));
	TEST_ASSERT_EQUAL(0xAC, mfnv1a("OpenValve"));
	TEST_ASSERT_EQUAL(0xAE, mfnv1a("CloseValve"));
	TEST_ASSERT_EQUAL(0x2C, mfnv1a("StoreDeployPoint"));
	TEST_ASSERT_EQUAL(0x20, mfnv1a("Disable"));
	TEST_ASSERT_EQUAL(0xD5, mfnv1a("Enable"));
	TEST_ASSERT_EQUAL(0xE6, mfnv1a("AccessGranted"));
	TEST_ASSERT_EQUAL(0x29, mfnv1a("GetNetworkStatus"));
	TEST_ASSERT_EQUAL(0x0E, mfnv1a("GetLog"));
}

