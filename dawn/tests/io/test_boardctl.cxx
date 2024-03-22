// dawn/tests/io/test_boardctl.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <fixedmath.h>

#include "dawn/io/boardctl.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

#ifndef CONFIG_DAWN_SIM_OVERLAY
#  error
#endif

// Reset test doesnt work now

#define DISABLE_BOARDCTL_POWEROFF_TEST ;

static uint32_t g_cfg_boardctl_reset[] = {
  CIOBoardctl::objectIdReset(),
  0,
};

static uint32_t g_cfg_boardctl_resetcause[] = {
  CIOBoardctl::objectIdResetCause(),
  0,
};

#ifndef DISABLE_BOARDCTL_POWEROFF_TEST
static uint32_t g_cfg_boardctl_poweroff[] = {
  CIOBoardctl::objectIdPoweroff(),
  0,
};
#endif

//***************************************************************************
// Description: test system IO reset
//***************************************************************************

static void test_io_system_reset()
{
  CDescObject desc(g_cfg_boardctl_reset);
  CIOBoardctl system(desc);
  io_sdata_t<uint32_t, 1> data;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, system.configure());
  TEST_ASSERT_EQUAL(OK, system.init());

  // Get size

  TEST_ASSERT_EQUAL(system.getDataSize(), data.getDataSize());

  // Not supported methods

  TEST_ASSERT_EQUAL(system.getData(data, 1), -ENOTSUP);
  TEST_ASSERT_EQUAL(system.getData(data, 2), -ENOTSUP);

  // Set data supported

  data(0) = 1;
  TEST_ASSERT_EQUAL(system.setData(data), OK);
}

//***************************************************************************
// Description: test system IO reset cause
//***************************************************************************

static void test_io_system_resetcause()
{
  CDescObject desc(g_cfg_boardctl_resetcause);
  CIOBoardctl system(desc);
  io_sdata_t<uint32_t, 2> data;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, system.configure());
  TEST_ASSERT_EQUAL(OK, system.init());

  // Get size

  TEST_ASSERT_EQUAL(system.getDataSize(), data.getDataSize());

  // Not supported methods

  TEST_ASSERT_EQUAL(system.setData(data), -ENOTSUP);
  TEST_ASSERT_EQUAL(system.getData(data, 2), -ENOTSUP);

  // Get data supported

  TEST_ASSERT_EQUAL(system.getData(data, 1), OK);

  // Cause and flag

  TEST_ASSERT_EQUAL(0xdeadbeef, data(0));
  TEST_ASSERT_EQUAL(0xbeefdead, data(1));
}

//***************************************************************************
// Description: test system IO power off
//***************************************************************************

static void test_io_system_poweroff()
{
#ifndef DISABLE_BOARDCTL_POWEROFF_TEST
  CDescObject desc(g_cfg_boardctl_poweroff);
  CIOBoardctl system(desc);
  io_sdata_t<uint32_t, 1> data;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, system.configure());
  TEST_ASSERT_EQUAL(OK, system.init());

  // Get size

  TEST_ASSERT_EQUAL(system.getDataSize(), data.getDataSize());

  // Not supported methods

  TEST_ASSERT_EQUAL(system.getData(data, 1), -ENOTSUP);
  TEST_ASSERT_EQUAL(system.getData(data, 2), -ENOTSUP);

  // Set data supported

  data(0) = 1;
  TEST_ASSERT_EQUAL(system.setData(data), OK);
#else
  // Disabled for now as it exit nuttx.
  // Weak symbol doens't work on board_power_off

  TEST_IGNORE_MESSAGE("disabled for now");
#endif
}

extern "C"
{
  int test_io_system_boardctl()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_system_reset);
    DAWN_RUN_TEST(test_io_system_resetcause);
    DAWN_RUN_TEST(test_io_system_poweroff);

    return UNITY_END();
  }
}
