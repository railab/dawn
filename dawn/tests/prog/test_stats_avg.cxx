// dawn/tests/prog/test_stats_avg.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/statsavg.hxx"
#include "test_process_common.hxx"

DEFINE_PROCESS_BIN(g_bin_stats_avg, CProgStatsAvg);

#define STATS_AVG_FIXTURE                         \
  CDescObject desc1(g_cfg_process_dummy1);        \
  CIODummyNotify src(desc1);                      \
  CDescObject desc2(g_cfg_process_virt1);         \
  CIOVirt virt(desc2);                            \
  CDescObject desc3(g_bin_stats_avg);             \
  CProgStatsAvg stats(desc3);                     \
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
// Description: feeding samples 0..3 produces a per-element running average.
//***************************************************************************

static void test_prog_stats_avg_running_avg()
{
  STATS_AVG_FIXTURE;
  io_sdata_t<uint32_t, 10, 1> data;

  process_set_dummy_data(src, 0);
  process_set_dummy_data(src, 1);
  process_set_dummy_data(src, 2);
  process_set_dummy_data(src, 3);

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(2, data(0));
  TEST_ASSERT_EQUAL(3, data(1));
  TEST_ASSERT_EQUAL(4, data(2));
  TEST_ASSERT_EQUAL(4, data(3));
  TEST_ASSERT_EQUAL(5, data(4));
  TEST_ASSERT_EQUAL(6, data(5));
  TEST_ASSERT_EQUAL(7, data(6));
  TEST_ASSERT_EQUAL(8, data(7));
  TEST_ASSERT_EQUAL(9, data(8));
  TEST_ASSERT_EQUAL(10, data(9));

  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, stats.stop());
}

//***************************************************************************
// Description: a CMD_RESET trigger clears the accumulator; the next sample
// alone determines the new average.
//***************************************************************************

static void test_prog_stats_avg_reset()
{
  STATS_AVG_FIXTURE;
  io_sdata_t<uint32_t, 10, 1> data;

  process_set_dummy_data(src, 0);
  process_set_dummy_data(src, 1);
  process_set_dummy_data(src, 2);
  process_set_dummy_data(src, 3);

  TEST_ASSERT_EQUAL(OK, stats.trigger(CObject::CMD_RESET));
  process_set_dummy_data(src, 4);

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(2, data(0));
  TEST_ASSERT_EQUAL(2, data(1));
  TEST_ASSERT_EQUAL(3, data(2));
  TEST_ASSERT_EQUAL(3, data(3));
  TEST_ASSERT_EQUAL(4, data(4));
  TEST_ASSERT_EQUAL(4, data(5));
  TEST_ASSERT_EQUAL(5, data(6));
  TEST_ASSERT_EQUAL(5, data(7));
  TEST_ASSERT_EQUAL(6, data(8));
  TEST_ASSERT_EQUAL(6, data(9));

  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, stats.stop());
}

extern "C"
{
  int test_prog_stats_avg()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_prog_stats_avg_running_avg);
    DAWN_RUN_TEST(test_prog_stats_avg_reset);
    return UNITY_END();
  }
}
