// dawn/tests/io/test_dac.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dac.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_dac0[] = {
  CIODac::objectId(false, 0),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

//***************************************************************************
// Description: DAC capability flags match the documented contract.
//***************************************************************************

static void test_io_dac_capabilities()
{
  CDescObject desc(g_cfg_dac0);
  CIODac dac(desc);

  TEST_ASSERT_FALSE(dac.isRead());
  TEST_ASSERT_TRUE(dac.isWrite());
  TEST_ASSERT_FALSE(dac.isNotify());
  TEST_ASSERT_FALSE(dac.isBatch());
  TEST_ASSERT_EQUAL(1u, dac.getDataDim());
}

//***************************************************************************
// Description: DAC rejects multi-item data on setData (single channel only).
//***************************************************************************

static void test_io_dac_setdata_multidim_rejected()
{
  CDescObject desc(g_cfg_dac0);
  CIODac dac(desc);
  io_sdata_t<int32_t, 2> data;

  data(0) = 100;
  data(1) = 200;

  TEST_ASSERT_EQUAL(-ENOMEM, dac.setData(data));
}

extern "C"
{
  int test_io_dac()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_dac_capabilities);
    DAWN_RUN_TEST(test_io_dac_setdata_multidim_rejected);

    return UNITY_END();
  }
}
