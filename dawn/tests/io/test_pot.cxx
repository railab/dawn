// dawn/tests/io/test_pot.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/pot.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_pot0[] = {
  CIOPot::objectId(false, 0),
  2,
  CIOCommon::cfgIdDevno(),
  0,
  CIOPot::cfgIdWiper(),
  1,
};

// Wiper item with two data words: only a single word is valid.

static uint32_t g_cfg_pot_badwiper[] = {
  CIOPot::objectId(false, 1),
  2,
  CIOCommon::cfgIdDevno(),
  0,
  CIOPot::cfgId(false, SObjectId::DTYPE_UINT32, 2, CIOPot::IO_POT_CFG_WIPER),
  1,
  0,
};

//***************************************************************************
// Description: POT capability flags match the documented contract.
//***************************************************************************

static void test_io_pot_capabilities()
{
  CDescObject desc(g_cfg_pot0);
  CIOPot pot(desc);

  TEST_ASSERT_TRUE(pot.isRead());
  TEST_ASSERT_TRUE(pot.isWrite());
  TEST_ASSERT_FALSE(pot.isNotify());
  TEST_ASSERT_FALSE(pot.isBatch());
  TEST_ASSERT_EQUAL(1u, pot.getDataDim());
  TEST_ASSERT_EQUAL(sizeof(int32_t), pot.getDataSize());
}

//***************************************************************************
// Description: POT rejects multi-item data on setData (single wiper only).
//***************************************************************************

static void test_io_pot_setdata_multidim_rejected()
{
  CDescObject desc(g_cfg_pot0);
  CIOPot pot(desc);
  io_sdata_t<int32_t, 2> data;

  data(0) = 100;
  data(1) = 200;

  TEST_ASSERT_EQUAL(-ENOMEM, pot.setData(data));
}

//***************************************************************************
// Description: POT rejects batched reads (len != 1).
//***************************************************************************

static void test_io_pot_getdata_batch_rejected()
{
  CDescObject desc(g_cfg_pot0);
  CIOPot pot(desc);
  io_sdata_t<int32_t, 1> data;

  TEST_ASSERT_EQUAL(-ENOTSUP, pot.getData(data, 2));
}

//***************************************************************************
// Description: configure() rejects a wiper config item whose size is not 1.
//***************************************************************************

static void test_io_pot_configure_rejects_wiper_size()
{
  CDescObject desc(g_cfg_pot_badwiper);
  CIOPot pot(desc);

  TEST_ASSERT_EQUAL(-EINVAL, pot.configure());
}

extern "C"
{
  int test_io_pot()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_pot_capabilities);
    DAWN_RUN_TEST(test_io_pot_setdata_multidim_rejected);
    DAWN_RUN_TEST(test_io_pot_getdata_batch_rejected);
    DAWN_RUN_TEST(test_io_pot_configure_rejects_wiper_size);

    return UNITY_END();
  }
}
