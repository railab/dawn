// dawn/tests/prog/test_stats_rms.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/statsrms.hxx"
#include "test_process_common.hxx"

DEFINE_PROCESS_BIN(g_bin_stats_rms, CProgStatsRms);

static uint32_t g_bin_stats_rms_multi[] = {
  CProgStatsRms::objectId(2),
  1,
  CProgStatsRms::cfgIdIOBind(4),
  PROCESS_DUMMYIO1,
  PROCESS_VIRTIO1,
  PROCESS_DUMMYIO2,
  PROCESS_VIRTIO2,
};

static constexpr auto RMS_SRC_INT32 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 31);
static constexpr auto RMS_DST_INT32 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 32);
static constexpr auto RMS_SRC_FLOAT = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 33);
static constexpr auto RMS_DST_FLOAT = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 34);
static constexpr auto RMS_SRC_U32_BIG = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 35);
static constexpr auto RMS_DST_U32_BIG = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 36);

static uint32_t g_cfg_rms_src_int32[] = {
  RMS_SRC_INT32,
  0,
};

static uint32_t g_cfg_rms_dst_int32[] = {
  RMS_DST_INT32,
  0,
};

static uint32_t g_cfg_rms_src_float[] = {
  RMS_SRC_FLOAT,
  0,
};

static uint32_t g_cfg_rms_dst_float[] = {
  RMS_DST_FLOAT,
  0,
};

static uint32_t g_cfg_rms_src_u32_big[] = {
  RMS_SRC_U32_BIG,
  0,
};

static uint32_t g_cfg_rms_dst_u32_big[] = {
  RMS_DST_U32_BIG,
  0,
};

static uint32_t g_bin_stats_rms_int32[] = {
  CProgStatsRms::objectId(10),
  1,
  CProgStatsRms::cfgIdIOBind(2),
  RMS_SRC_INT32,
  RMS_DST_INT32,
};

static uint32_t g_bin_stats_rms_float[] = {
  CProgStatsRms::objectId(11),
  1,
  CProgStatsRms::cfgIdIOBind(2),
  RMS_SRC_FLOAT,
  RMS_DST_FLOAT,
};

static uint32_t g_bin_stats_rms_u32_big[] = {
  CProgStatsRms::objectId(12),
  1,
  CProgStatsRms::cfgIdIOBind(2),
  RMS_SRC_U32_BIG,
  RMS_DST_U32_BIG,
};

//***************************************************************************
// Description: RMS stats updates every output element and resets state.
//***************************************************************************

static void test_prog_stats_rms_all()
{
  CDescObject desc1(g_cfg_process_dummy1);
  CIODummyNotify src(desc1);
  CDescObject desc2(g_cfg_process_virt1);
  CIOVirt virt(desc2);
  CDescObject desc3(g_bin_stats_rms);
  CProgStatsRms stats(desc3);
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

  for (int i = 0; i < 4; i++)
    {
      process_set_dummy_data(src, i);
    }

  TEST_ASSERT_EQUAL(OK, virt.getData(data, 1));
  TEST_ASSERT_EQUAL(1, data(0));
  TEST_ASSERT_EQUAL(2, data(1));
  TEST_ASSERT_EQUAL(3, data(2));
  TEST_ASSERT_EQUAL(4, data(3));
  TEST_ASSERT_EQUAL(5, data(4));
  TEST_ASSERT_EQUAL(6, data(5));
  TEST_ASSERT_EQUAL(7, data(6));
  TEST_ASSERT_EQUAL(8, data(7));
  TEST_ASSERT_EQUAL(9, data(8));
  TEST_ASSERT_EQUAL(10, data(9));

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

  ret = notifier.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = stats.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: RMS stats handles two independent source/output bindings.
//***************************************************************************

static void test_prog_stats_rms_multi_bind()
{
  CDescObject desc1(g_cfg_process_dummy1);
  CIODummyNotify src1(desc1);
  CDescObject desc1b(g_cfg_process_dummy2);
  CIODummyNotify src2(desc1b);
  CDescObject desc2(g_cfg_process_virt1);
  CIOVirt virt1(desc2);
  CDescObject desc2b(g_cfg_process_virt2);
  CIOVirt virt2(desc2b);
  CDescObject desc3(g_bin_stats_rms_multi);
  CProgStatsRms stats(desc3);
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
  TEST_ASSERT_EQUAL(1, data1(0));
  TEST_ASSERT_EQUAL(10, data1(9));
  TEST_ASSERT_EQUAL(1, data2(0));
  TEST_ASSERT_EQUAL(10, data2(9));

  TEST_ASSERT_EQUAL(OK, stats.trigger(CObject::CMD_RESET));
  process_set_dummy_data(src1, 4);
  process_set_dummy_data(src2, 4);

  TEST_ASSERT_EQUAL(OK, virt1.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, virt2.getData(data2, 1));
  TEST_ASSERT_EQUAL(4, data1(0));
  TEST_ASSERT_EQUAL(13, data1(9));
  TEST_ASSERT_EQUAL(4, data2(0));
  TEST_ASSERT_EQUAL(13, data2(9));

  ret = notifier.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = stats.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: int32 RMS handles negative samples and reset.
//***************************************************************************

static void test_prog_stats_rms_int32_negative_and_reset()
{
  CDescObject srcDesc(g_cfg_rms_src_int32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_rms_dst_int32);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_stats_rms_int32);
  CProgStatsRms stats(progDesc);
  io_sdata_t<int32_t, 1, 1> in;
  io_sdata_t<int32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, stats.configure());
  stats.setObjectMapItem(RMS_SRC_INT32, &src);
  stats.setObjectMapItem(RMS_DST_INT32, &dst);
  TEST_ASSERT_EQUAL(OK, stats.init());
  TEST_ASSERT_EQUAL(OK, stats.start());

  in(0) = -3;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(3, out(0));

  in(0) = 4;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(3, out(0)); // floor(sqrt((9+16)/2)) = 3

  in(0) = -4;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(3, out(0)); // floor(sqrt((9+16+16)/3)) = 3

  TEST_ASSERT_EQUAL(OK, stats.trigger(CObject::CMD_RESET));
  in(0) = -12;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(12, out(0));

  TEST_ASSERT_EQUAL(OK, stats.stop());
}

//***************************************************************************
// Description: float RMS accumulates samples and resets state.
//***************************************************************************

static void test_prog_stats_rms_float_path()
{
  CDescObject srcDesc(g_cfg_rms_src_float);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_rms_dst_float);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_stats_rms_float);
  CProgStatsRms stats(progDesc);
  io_sdata_t<float, 1, 1> in;
  io_sdata_t<float, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, stats.configure());
  stats.setObjectMapItem(RMS_SRC_FLOAT, &src);
  stats.setObjectMapItem(RMS_DST_FLOAT, &dst);
  TEST_ASSERT_EQUAL(OK, stats.init());
  TEST_ASSERT_EQUAL(OK, stats.start());

  in(0) = 3.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL_FLOAT(3.0f, out(0));

  in(0) = 4.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.5355339f, out(0));

  TEST_ASSERT_EQUAL(OK, stats.trigger(CObject::CMD_RESET));
  in(0) = -5.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 5.0f, out(0));

  TEST_ASSERT_EQUAL(OK, stats.stop());
}

//***************************************************************************
// Description: uint32 RMS handles large values without truncation.
//***************************************************************************

static void test_prog_stats_rms_uint32_large_value()
{
  CDescObject srcDesc(g_cfg_rms_src_u32_big);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_rms_dst_u32_big);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_stats_rms_u32_big);
  CProgStatsRms stats(progDesc);
  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, stats.configure());
  stats.setObjectMapItem(RMS_SRC_U32_BIG, &src);
  stats.setObjectMapItem(RMS_DST_U32_BIG, &dst);
  TEST_ASSERT_EQUAL(OK, stats.init());
  TEST_ASSERT_EQUAL(OK, stats.start());

  in(0) = 70000u;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(70000u, out(0));

  TEST_ASSERT_EQUAL(OK, stats.stop());
}

extern "C"
{
  int test_prog_stats_rms()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_prog_stats_rms_all);
    DAWN_RUN_TEST(test_prog_stats_rms_multi_bind);
    DAWN_RUN_TEST(test_prog_stats_rms_int32_negative_and_reset);
    DAWN_RUN_TEST(test_prog_stats_rms_float_path);
    DAWN_RUN_TEST(test_prog_stats_rms_uint32_large_value);
    return UNITY_END();
  }
}
