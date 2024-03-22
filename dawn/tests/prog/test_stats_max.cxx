// dawn/tests/prog/test_stats_max.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/statsmax.hxx"
#include "test_process_common.hxx"

DEFINE_PROCESS_BIN(g_bin_stats_max, CProgStatsMax);

#define STATS_MAX_FIXTURE                         \
  CDescObject desc1(g_cfg_process_dummy1);        \
  CIODummyNotify src(desc1);                      \
  CDescObject desc2(g_cfg_process_virt1);         \
  CIOVirt virt(desc2);                            \
  CDescObject desc3(g_bin_stats_max);             \
  CProgStatsMax stats(desc3);                     \
  CIONotifier notifier;                           \
  TEST_ASSERT_EQUAL(OK, src.configure());         \
  TEST_ASSERT_EQUAL(OK, src.init());              \
  TEST_ASSERT_EQUAL(OK, virt.init());             \
  TEST_ASSERT_EQUAL(OK, stats.configure());       \
  src.bindNotifier(&notifier);                    \
  stats.setObjectMapItem(PROCESS_DUMMYIO1, &src); \
  stats.setObjectMapItem(PROCESS_VIRTIO1, &virt); \
  TEST_ASSERT_EQUAL(OK, stats.init());            \
  TEST_ASSERT_EQUAL(OK, stats.start());           \
  TEST_ASSERT_EQUAL(OK, notifier.start())

//***************************************************************************
// Description: feeding ascending samples 0..3 yields the largest seen
// per channel (the last sample, which is original_value + 3).
//***************************************************************************

static void test_prog_stats_max_running_max()
{
  STATS_MAX_FIXTURE;
  io_sdata_t<uint32_t, 10, 1> data;

  process_set_dummy_data(src, 0);
  process_set_dummy_data(src, 1);
  process_set_dummy_data(src, 2);
  process_set_dummy_data(src, 3);

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(0 + 3, data(0));
  TEST_ASSERT_EQUAL(1 + 3, data(1));
  TEST_ASSERT_EQUAL(2 + 3, data(2));
  TEST_ASSERT_EQUAL(3 + 3, data(3));
  TEST_ASSERT_EQUAL(4 + 3, data(4));
  TEST_ASSERT_EQUAL(5 + 3, data(5));
  TEST_ASSERT_EQUAL(6 + 3, data(6));
  TEST_ASSERT_EQUAL(7 + 3, data(7));
  TEST_ASSERT_EQUAL(8 + 3, data(8));
  TEST_ASSERT_EQUAL(9 + 3, data(9));

  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, stats.stop());
}

//***************************************************************************
// Description: a CMD_RESET trigger clears the recorded maxima; the next
// sample becomes the new maximum.
//***************************************************************************

static void test_prog_stats_max_reset()
{
  STATS_MAX_FIXTURE;
  io_sdata_t<uint32_t, 10, 1> data;

  process_set_dummy_data(src, 0);
  process_set_dummy_data(src, 1);
  process_set_dummy_data(src, 2);
  process_set_dummy_data(src, 3);

  TEST_ASSERT_EQUAL(OK, stats.trigger(CObject::CMD_RESET));
  process_set_dummy_data(src, 4);

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(4, data(0));
  TEST_ASSERT_EQUAL(5, data(1));
  TEST_ASSERT_EQUAL(6, data(2));
  TEST_ASSERT_EQUAL(7, data(3));
  TEST_ASSERT_EQUAL(8, data(4));
  TEST_ASSERT_EQUAL(9, data(5));
  TEST_ASSERT_EQUAL(10, data(6));
  TEST_ASSERT_EQUAL(11, data(7));
  TEST_ASSERT_EQUAL(12, data(8));
  TEST_ASSERT_EQUAL(13, data(9));

  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, stats.stop());
}

extern "C"
{
  int test_prog_stats_max()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_prog_stats_max_running_max);
    DAWN_RUN_TEST(test_prog_stats_max_reset);
    return UNITY_END();
  }
}
