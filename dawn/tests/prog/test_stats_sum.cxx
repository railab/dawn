// dawn/tests/prog/test_stats_sum.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/statssum.hxx"
#include "test_process_common.hxx"

DEFINE_PROCESS_BIN(g_bin_stats_sum, CProgStatsSum);

static constexpr auto STATS_SUM_SRC_U16 = CIOVirt::objectId(SObjectId::DTYPE_UINT16, false, 61);
static constexpr auto STATS_SUM_DST_U16 = CIOVirt::objectId(SObjectId::DTYPE_UINT16, false, 62);

static uint32_t g_cfg_stats_sum_src_u16[] = {
  STATS_SUM_SRC_U16,
  0,
};

static uint32_t g_cfg_stats_sum_dst_u16[] = {
  STATS_SUM_DST_U16,
  0,
};

static uint32_t g_bin_stats_sum_u16[] = {
  CProgStatsSum::objectId(1),
  1,
  CProgStatsSum::cfgIdIOBind(2),
  STATS_SUM_SRC_U16,
  STATS_SUM_DST_U16,
};

// Configure + init src, virt, and stats program; bind notifier and IO map;
// start prog + notifier.  Caller drives setData on src and stops both.

#define STATS_SUM_FIXTURE                         \
  CDescObject desc1(g_cfg_process_dummy1);        \
  CIODummyNotify src(desc1);                      \
  CDescObject desc2(g_cfg_process_virt1);         \
  CIOVirt virt(desc2);                            \
  CDescObject desc3(g_bin_stats_sum);             \
  CProgStatsSum stats(desc3);                     \
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
// Description: feeding samples 0..3 produces a per-element running sum
// (6, 10, 14, ... 42 for the 10 channels).
//***************************************************************************

static void test_prog_stats_sum_running_sum()
{
  STATS_SUM_FIXTURE;
  io_sdata_t<uint32_t, 10, 1> data;

  process_set_dummy_data(src, 0);
  process_set_dummy_data(src, 1);
  process_set_dummy_data(src, 2);
  process_set_dummy_data(src, 3);

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(6, data(0));
  TEST_ASSERT_EQUAL(10, data(1));
  TEST_ASSERT_EQUAL(14, data(2));
  TEST_ASSERT_EQUAL(18, data(3));
  TEST_ASSERT_EQUAL(22, data(4));
  TEST_ASSERT_EQUAL(26, data(5));
  TEST_ASSERT_EQUAL(30, data(6));
  TEST_ASSERT_EQUAL(34, data(7));
  TEST_ASSERT_EQUAL(38, data(8));
  TEST_ASSERT_EQUAL(42, data(9));

  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, stats.stop());
}

//***************************************************************************
// Description: a CMD_RESET trigger zeroes the accumulator; the next sample
// produces values equal to that sample alone.
//***************************************************************************

static void test_prog_stats_sum_reset()
{
  STATS_SUM_FIXTURE;
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

//***************************************************************************
// Description: uint16 samples use the shared numeric dispatch path.
//***************************************************************************

static void test_prog_stats_sum_uint16()
{
  CDescObject srcDesc(g_cfg_stats_sum_src_u16);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_stats_sum_dst_u16);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_stats_sum_u16);
  CProgStatsSum stats(progDesc);
  io_sdata_t<uint16_t, 1, 1> in;
  io_sdata_t<uint16_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, stats.configure());
  stats.setObjectMapItem(STATS_SUM_SRC_U16, &src);
  stats.setObjectMapItem(STATS_SUM_DST_U16, &dst);
  TEST_ASSERT_EQUAL(OK, stats.init());
  TEST_ASSERT_EQUAL(OK, stats.start());

  in(0) = 2;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 5;
  TEST_ASSERT_EQUAL(OK, src.setData(in));

  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(7, out(0));

  TEST_ASSERT_EQUAL(OK, stats.stop());
}

extern "C"
{
  int test_prog_stats_sum()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_prog_stats_sum_running_sum);
    DAWN_RUN_TEST(test_prog_stats_sum_reset);
    DAWN_RUN_TEST(test_prog_stats_sum_uint16);
    return UNITY_END();
  }
}
