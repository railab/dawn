// dawn/tests/io/test_gpo.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/gpo.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: test descriptors for GPO
//***************************************************************************

static uint32_t g_cfg_gpo0[] = {
  // Device: /dev/gpio4

  CIOGpo::objectId(false, 5),
  1,
  CIOCommon::cfgIdDevno(),
  4,
};

static uint32_t g_cfg_gpo1[] = {
  // Device: /dev/gpio5

  CIOGpo::objectId(false, 6),
  1,
  CIOCommon::cfgIdDevno(),
  5,
};

static uint32_t g_cfg_gpo2[] = {
  // Device: /dev/gpio6

  CIOGpo::objectId(true, 7),
  1,
  CIOCommon::cfgIdDevno(),
  6,
};

//***************************************************************************
// Description: a fresh GPO without timestamp reads as low (0).
//***************************************************************************

static void test_io_gpo_initial_low()
{
  CDescObject desc(g_cfg_gpo0);
  CIOGpo gpo(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, gpo.configure());
  TEST_ASSERT_EQUAL(OK, gpo.init());

  TEST_ASSERT_EQUAL(OK, gpo.getData(data, 1));
  TEST_ASSERT_EQUAL(false, data(0));
  TEST_ASSERT_EQUAL(0, data[0]);
}

//***************************************************************************
// Description: setData(true) followed by getData returns the new value.
//***************************************************************************

static void test_io_gpo_set_high_reads_high()
{
  CDescObject desc(g_cfg_gpo0);
  CIOGpo gpo(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, gpo.configure());
  TEST_ASSERT_EQUAL(OK, gpo.init());

  data(0) = true;
  TEST_ASSERT_EQUAL(OK, gpo.setData(data));

  TEST_ASSERT_EQUAL(OK, gpo.getData(data, 1));
  TEST_ASSERT_EQUAL(true, data(0));
}

//***************************************************************************
// Description: a GPO can be toggled high then low and getData reflects
// each transition.
//***************************************************************************

static void test_io_gpo_toggle_high_low()
{
  CDescObject desc(g_cfg_gpo1);
  CIOGpo gpo(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, gpo.configure());
  TEST_ASSERT_EQUAL(OK, gpo.init());

  data(0) = true;
  TEST_ASSERT_EQUAL(OK, gpo.setData(data));
  TEST_ASSERT_EQUAL(OK, gpo.getData(data, 1));
  TEST_ASSERT_EQUAL(true, data(0));

  data(0) = false;
  TEST_ASSERT_EQUAL(OK, gpo.setData(data));
  TEST_ASSERT_EQUAL(OK, gpo.getData(data, 1));
  TEST_ASSERT_EQUAL(false, data(0));
}

//***************************************************************************
// Description: a GPO with timestamps starts with ts=0 (no setData yet).
//***************************************************************************

static void test_io_gpo_ts_initial_zero()
{
  CDescObject desc(g_cfg_gpo2);
  CIOGpo gpo(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, gpo.configure());
  TEST_ASSERT_EQUAL(OK, gpo.init());

  TEST_ASSERT_EQUAL(OK, gpo.getData(data, 1));
  TEST_ASSERT_EQUAL(false, data(0));
  TEST_ASSERT_EQUAL(0, data[0]);
}

//***************************************************************************
// Description: each setData on a timestamp-enabled GPO produces a strictly
// increasing timestamp.
//***************************************************************************

static void test_io_gpo_ts_advances_on_set()
{
  CDescObject desc(g_cfg_gpo2);
  CIOGpo gpo(desc);
  io_sdata_t<uint32_t, 1> data;
  uint64_t ts1;

  TEST_ASSERT_EQUAL(OK, gpo.configure());
  TEST_ASSERT_EQUAL(OK, gpo.init());

  data(0) = true;
  TEST_ASSERT_EQUAL(OK, gpo.setData(data));
  TEST_ASSERT_EQUAL(OK, gpo.getData(data, 1));
  TEST_ASSERT(data[0] != 0);
  ts1 = data[0];

  usleep(1);

  data(0) = false;
  TEST_ASSERT_EQUAL(OK, gpo.setData(data));
  TEST_ASSERT_EQUAL(OK, gpo.getData(data, 1));
  TEST_ASSERT(data[0] > ts1);
}

extern "C"
{
  int test_io_gpo()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_gpo_initial_low);
    DAWN_RUN_TEST(test_io_gpo_set_high_reads_high);
    DAWN_RUN_TEST(test_io_gpo_toggle_high_low);
    DAWN_RUN_TEST(test_io_gpo_ts_initial_zero);
    DAWN_RUN_TEST(test_io_gpo_ts_advances_on_set);

    return UNITY_END();
  }
}
