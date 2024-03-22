// dawn/tests/prog/test_stats_count.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/statscount.hxx"
#include "test_process_common.hxx"

DEFINE_PROCESS_BIN(g_bin_stats_count, CProgStatsCount);

static uint32_t g_bin_stats_count_multi[] = {
  CProgStatsCount::objectId(2),
  1,
  CProgStatsCount::cfgIdIOBind(4),
  PROCESS_DUMMYIO1,
  PROCESS_VIRTIO1,
  PROCESS_DUMMYIO2,
  PROCESS_VIRTIO2,
};

//***************************************************************************
// Description: stats-count updates every output element and resets count.
//***************************************************************************

static void test_prog_stats_count_all()
{
  CDescObject desc1(g_cfg_process_dummy1);
  CIODummyNotify src(desc1);
  CDescObject desc2(g_cfg_process_virt1);
  CIOVirt virt(desc2);
  CDescObject desc3(g_bin_stats_count);
  CProgStatsCount stats(desc3);
  CIONotifier notifier;
  io_sdata_t<uint32_t, 10, 1> data;
  int ret;

  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, stats.configure());
  src.bindNotifier(&notifier);
  stats.setObjectMapItem(PROCESS_DUMMYIO1, &src);
  stats.setObjectMapItem(PROCESS_VIRTIO1, &virt);
  TEST_ASSERT_EQUAL(OK, stats.init());

  ret = stats.start();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = notifier.start();
  TEST_ASSERT_EQUAL(OK, ret);

  process_set_dummy_data(src, 0);
  process_set_dummy_data(src, 1);
  process_set_dummy_data(src, 2);
  process_set_dummy_data(src, 3);

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(4, data(0));
  TEST_ASSERT_EQUAL(4, data(1));
  TEST_ASSERT_EQUAL(4, data(2));
  TEST_ASSERT_EQUAL(4, data(3));
  TEST_ASSERT_EQUAL(4, data(4));
  TEST_ASSERT_EQUAL(4, data(5));
  TEST_ASSERT_EQUAL(4, data(6));
  TEST_ASSERT_EQUAL(4, data(7));
  TEST_ASSERT_EQUAL(4, data(8));
  TEST_ASSERT_EQUAL(4, data(9));

  TEST_ASSERT_EQUAL(OK, stats.trigger(CObject::CMD_RESET));
  process_set_dummy_data(src, 4);

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(1, data(0));
  TEST_ASSERT_EQUAL(1, data(1));
  TEST_ASSERT_EQUAL(1, data(2));
  TEST_ASSERT_EQUAL(1, data(3));
  TEST_ASSERT_EQUAL(1, data(4));
  TEST_ASSERT_EQUAL(1, data(5));
  TEST_ASSERT_EQUAL(1, data(6));
  TEST_ASSERT_EQUAL(1, data(7));
  TEST_ASSERT_EQUAL(1, data(8));
  TEST_ASSERT_EQUAL(1, data(9));

  ret = notifier.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = stats.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: stats-count handles two independent source/output bindings.
//***************************************************************************

static void test_prog_stats_count_multi_bind()
{
  CDescObject desc1(g_cfg_process_dummy1);
  CIODummyNotify src1(desc1);
  CDescObject desc1b(g_cfg_process_dummy2);
  CIODummyNotify src2(desc1b);
  CDescObject desc2(g_cfg_process_virt1);
  CIOVirt virt1(desc2);
  CDescObject desc2b(g_cfg_process_virt2);
  CIOVirt virt2(desc2b);
  CDescObject desc3(g_bin_stats_count_multi);
  CProgStatsCount stats(desc3);
  CIONotifier notifier;
  io_sdata_t<uint32_t, 10, 1> data1;
  io_sdata_t<uint32_t, 10, 1> data2;
  int ret;

  TEST_ASSERT_EQUAL(OK, src1.configure());
  TEST_ASSERT_EQUAL(OK, src1.init());
  TEST_ASSERT_EQUAL(OK, src2.configure());
  TEST_ASSERT_EQUAL(OK, src2.init());
  TEST_ASSERT_EQUAL(OK, virt1.init());
  TEST_ASSERT_EQUAL(OK, virt2.init());
  TEST_ASSERT_EQUAL(OK, stats.configure());
  src1.bindNotifier(&notifier);
  src2.bindNotifier(&notifier);
  stats.setObjectMapItem(PROCESS_DUMMYIO1, &src1);
  stats.setObjectMapItem(PROCESS_VIRTIO1, &virt1);
  stats.setObjectMapItem(PROCESS_DUMMYIO2, &src2);
  stats.setObjectMapItem(PROCESS_VIRTIO2, &virt2);
  TEST_ASSERT_EQUAL(OK, stats.init());

  ret = stats.start();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = notifier.start();
  TEST_ASSERT_EQUAL(OK, ret);

  for (int i = 0; i < 4; i++)
    {
      process_set_dummy_data(src1, i);
      process_set_dummy_data(src2, i);
    }

  TEST_ASSERT_EQUAL(OK, virt1.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, virt2.getData(data2, 1));
  TEST_ASSERT_EQUAL(4, data1(0));
  TEST_ASSERT_EQUAL(4, data1(9));
  TEST_ASSERT_EQUAL(4, data2(0));
  TEST_ASSERT_EQUAL(4, data2(9));

  TEST_ASSERT_EQUAL(OK, stats.trigger(CObject::CMD_RESET));
  process_set_dummy_data(src1, 4);
  process_set_dummy_data(src2, 4);

  TEST_ASSERT_EQUAL(OK, virt1.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, virt2.getData(data2, 1));
  TEST_ASSERT_EQUAL(1, data1(0));
  TEST_ASSERT_EQUAL(1, data2(0));

  ret = notifier.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = stats.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

extern "C"
{
  int test_prog_stats_count()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_prog_stats_count_all);
    DAWN_RUN_TEST(test_prog_stats_count_multi_bind);
    return UNITY_END();
  }
}
