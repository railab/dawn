// dawn/tests/io/test_battery.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>

#include "dawn/io/battery.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

// Battery voltage IO, devno 0.

static uint32_t g_cfg_batt_volt[] = {
  CIOBattVolt::objectId(false, 0),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

// Battery state-of-charge IO, devno 0.

static uint32_t g_cfg_batt_soc[] = {
  CIOBattSoc::objectId(false, 0),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

// Battery charge-state IO, devno 0.

static uint32_t g_cfg_batt_state[] = {
  CIOBattState::objectId(false, 0),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

//***************************************************************************
// Description: the battery IOs are read-only, scalar DTYPE_UINT32 IOs whose
// data size matches a single uint32.
//***************************************************************************

static void test_io_battery_metadata()
{
  CDescObject descVolt(g_cfg_batt_volt);
  CIOBattVolt volt(descVolt);
  CDescObject descSoc(g_cfg_batt_soc);
  CIOBattSoc soc(descSoc);
  CDescObject descState(g_cfg_batt_state);
  CIOBattState state(descState);
  io_sdata_t<uint32_t, 1> data;

  CIOCommon *ios[] = {&volt, &soc, &state};
  for (CIOCommon *io : ios)
    {
      TEST_ASSERT_EQUAL(SObjectId::DTYPE_UINT32, io->getDtype());
      TEST_ASSERT_EQUAL(data.getDataSize(), io->getDataSize());
      TEST_ASSERT_EQUAL(1, io->getDataDim());
      TEST_ASSERT_TRUE(io->isRead());
      TEST_ASSERT_FALSE(io->isWrite());
    }
}

//***************************************************************************
// Description: configuring a battery IO fails (rather than crashing) when no
// battery_gauge device is present.
//***************************************************************************

static void test_io_battery_configure_no_device()
{
  CDescObject desc(g_cfg_batt_volt);
  CIOBattVolt volt(desc);

  TEST_ASSERT_TRUE(volt.configure() < 0);
}

extern "C"
{
  int test_io_battery()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_battery_metadata);
    DAWN_RUN_TEST(test_io_battery_configure_no_device);

    return UNITY_END();
  }
}
