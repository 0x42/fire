/* AUTOGENERATED FILE. DO NOT EDIT. */
#include "unity.h"
#include <setjmp.h>
#include <stdio.h>

char MessageBuffer[50];

extern void setUp(void);
extern void tearDown(void);

extern void test_FindFunction_WhichIsBroken_ShouldReturnZeroIfItemIsNotInList_WhichWorksEvenInOurBrokenCode(void);
extern void test_FindFunction_WhichIsBroken_ShouldReturnTheIndexForItemsInList_WhichWillFailBecauseOurFunctionUnderTestIsBroken(void);
extern void test_FunctionWhichReturnsLocalVariable_ShouldReturnTheCurrentCounterValue(void);
extern void test_FunctionWhichReturnsLocalVariable_ShouldReturnTheCurrentCounterValueAgain(void);
extern void test_FunctionWhichReturnsLocalVariable_ShouldReturnCurrentCounter_ButFailsBecauseThisTestIsActuallyFlawed(void);

static void runTest(UnityTestFunction test)
{
  if (TEST_PROTECT())
  {
      setUp();
      test();
  }
  if (TEST_PROTECT() && !TEST_IS_IGNORED)
  {
    tearDown();
  }
}
void resetTest()
{
  tearDown();
  setUp();
}


int main(void)
{
  Unity.TestFile = "test/TestProductionCode.c";
  UnityBegin();

  // RUN_TEST calls runTest
  RUN_TEST(test_FindFunction_WhichIsBroken_ShouldReturnZeroIfItemIsNotInList_WhichWorksEvenInOurBrokenCode, 20);
  RUN_TEST(test_FindFunction_WhichIsBroken_ShouldReturnTheIndexForItemsInList_WhichWillFailBecauseOurFunctionUnderTestIsBroken, 30);
  RUN_TEST(test_FunctionWhichReturnsLocalVariable_ShouldReturnTheCurrentCounterValue, 41);
  RUN_TEST(test_FunctionWhichReturnsLocalVariable_ShouldReturnTheCurrentCounterValueAgain, 51);
  RUN_TEST(test_FunctionWhichReturnsLocalVariable_ShouldReturnCurrentCounter_ButFailsBecauseThisTestIsActuallyFlawed, 57);

  UnityEnd();
  return 0;
}
