// dawn/tests/common/test_objectcfg.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/objectcfg.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: object config bitfields round-trip through accessor helpers.
//***************************************************************************

static void test_common_objectcfg_init()
{
  SObjectCfg::ObjectCfgId cfg1 = SObjectCfg::objectCfg(1, 4, 3, 1, 2, 1);
  SObjectCfg::ObjectCfgId cfg2 = SObjectCfg::objectCfg(2, 5, 6, 0, 3, 6);

  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetType(cfg1), 1);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetCls(cfg1), 4);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetDtype(cfg1), 3);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetRw(cfg1), 1);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetSize(cfg1), 2);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetId(cfg1), 1);

  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetType(cfg2), 2);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetCls(cfg2), 5);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetDtype(cfg2), 6);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetRw(cfg2), 0);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetSize(cfg2), 3);
  TEST_ASSERT_EQUAL(SObjectCfg::objectCfgGetId(cfg2), 6);
}

//***************************************************************************
// Description: test data convertion
//***************************************************************************

static void test_common_objectcfg_convert()
{
  TEST_ASSERT_EQUAL(0, SObjectCfg::u32ToCfg(0));
  TEST_ASSERT_EQUAL(0, SObjectCfg::i32ToCfg(0));
  TEST_ASSERT_EQUAL(0, SObjectCfg::fToCfg(0.0f));
  TEST_ASSERT_EQUAL(0, SObjectCfg::b16ToCfg(0));

  TEST_ASSERT_EQUAL(1, SObjectCfg::u32ToCfg(1));
  TEST_ASSERT_EQUAL(1, SObjectCfg::i32ToCfg(1));
  TEST_ASSERT_EQUAL(0x3F800000, SObjectCfg::fToCfg(1.0f));
  TEST_ASSERT_EQUAL(b16ONE, SObjectCfg::fToB16ToCfg(1.0f));
  TEST_ASSERT_EQUAL(b16ONE, SObjectCfg::b16ToCfg(b16ONE));

  TEST_ASSERT_EQUAL(1000, SObjectCfg::u32ToCfg(1000));
  TEST_ASSERT_EQUAL(1000, SObjectCfg::i32ToCfg(1000));
  TEST_ASSERT_EQUAL(0x447A0000, SObjectCfg::fToCfg(1000.0f));
  TEST_ASSERT_EQUAL(b16THOUSAND, SObjectCfg::fToB16ToCfg(1000.0f));
  TEST_ASSERT_EQUAL(b16THOUSAND, SObjectCfg::b16ToCfg(b16THOUSAND));

  // Negative values

  TEST_ASSERT_EQUAL(0xFFFFFC18, SObjectCfg::i32ToCfg(-1000));
  TEST_ASSERT_EQUAL(0xC47A0000, SObjectCfg::fToCfg(-1000.0f));
  TEST_ASSERT_EQUAL(0xfc180000, SObjectCfg::fToB16ToCfg(-1000.0f));
  TEST_ASSERT_EQUAL(0xfc180000, SObjectCfg::b16ToCfg(-b16THOUSAND));
}

extern "C"
{
  int test_common_objectcfg()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_common_objectcfg_init);
    DAWN_RUN_TEST(test_common_objectcfg_convert);

    return UNITY_END();
  }
}
