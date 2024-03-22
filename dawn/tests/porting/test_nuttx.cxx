// dawn/tests/porting/test_nuttx.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/config.hxx"
#include "test_common.hxx"

//***************************************************************************
// Description: nothing is here for now
//***************************************************************************

static void test_test1()
{
  TEST_IGNORE_MESSAGE("TODO");
}

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: test_nuttx
//***************************************************************************

extern "C"
{
  int test_nuttx()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_test1);

    return UNITY_END();
  }
}
