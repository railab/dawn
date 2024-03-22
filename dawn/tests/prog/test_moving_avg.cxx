// dawn/tests/prog/test_moving_avg.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/movingavg.hxx"
#include "test_process_common.hxx"

using namespace dawn;

static constexpr auto MOVAVG_SRC_I32 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 41);
static constexpr auto MOVAVG_DST_I32 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 42);
static constexpr auto MOVAVG_SRC_F32 = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 43);
static constexpr auto MOVAVG_DST_F32 = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 44);
static constexpr auto MOVAVG_SRC_U16 = CIOVirt::objectId(SObjectId::DTYPE_UINT16, false, 45);
static constexpr auto MOVAVG_DST_U16 = CIOVirt::objectId(SObjectId::DTYPE_UINT16, false, 46);
static constexpr auto MOVAVG_SRC_F64 = CIOVirt::objectId(SObjectId::DTYPE_DOUBLE, false, 47);
static constexpr auto MOVAVG_DST_F64 = CIOVirt::objectId(SObjectId::DTYPE_DOUBLE, false, 48);

static uint32_t g_cfg_movavg_src_i32[] = {
  MOVAVG_SRC_I32,
  0,
};

static uint32_t g_cfg_movavg_dst_i32[] = {
  MOVAVG_DST_I32,
  0,
};

static uint32_t g_cfg_movavg_src_f32[] = {
  MOVAVG_SRC_F32,
  0,
};

static uint32_t g_cfg_movavg_dst_f32[] = {
  MOVAVG_DST_F32,
  0,
};

static uint32_t g_cfg_movavg_src_u16[] = {
  MOVAVG_SRC_U16,
  0,
};

static uint32_t g_cfg_movavg_dst_u16[] = {
  MOVAVG_DST_U16,
  0,
};

static uint32_t g_cfg_movavg_src_f64[] = {
  MOVAVG_SRC_F64,
  0,
};

static uint32_t g_cfg_movavg_dst_f64[] = {
  MOVAVG_DST_F64,
  0,
};

static uint32_t g_bin_movavg_i32[] = {
  CProgMovingAverage::objectId(0),
  2,
  CProgMovingAverage::cfgIdIOBind(2),
  MOVAVG_SRC_I32,
  MOVAVG_DST_I32,
  CProgMovingAverage::cfgIdWindow(),
  4,
};

static uint32_t g_bin_movavg_f32[] = {
  CProgMovingAverage::objectId(2),
  2,
  CProgMovingAverage::cfgIdIOBind(2),
  MOVAVG_SRC_F32,
  MOVAVG_DST_F32,
  CProgMovingAverage::cfgIdWindow(),
  3,
};

static uint32_t g_bin_movavg_u16[] = {
  CProgMovingAverage::objectId(3),
  2,
  CProgMovingAverage::cfgIdIOBind(2),
  MOVAVG_SRC_U16,
  MOVAVG_DST_U16,
  CProgMovingAverage::cfgIdWindow(),
  2,
};

static uint32_t g_bin_movavg_f64[] = {
  CProgMovingAverage::objectId(4),
  2,
  CProgMovingAverage::cfgIdIOBind(2),
  MOVAVG_SRC_F64,
  MOVAVG_DST_F64,
  CProgMovingAverage::cfgIdWindow(),
  2,
};

//***************************************************************************
// Description: int32 moving average honors window size and reset.
//***************************************************************************

static void test_prog_moving_avg_int32_window_and_reset()
{
  CDescObject srcDesc(g_cfg_movavg_src_i32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_movavg_dst_i32);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_movavg_i32);
  CProgMovingAverage prog(progDesc);
  io_sdata_t<int32_t, 1, 1> in;
  io_sdata_t<int32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(MOVAVG_SRC_I32, &src);
  prog.setObjectMapItem(MOVAVG_DST_I32, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 10;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(10, out(0));

  in(0) = 20;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(15, out(0));

  in(0) = 30;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(20, out(0));

  in(0) = 40;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(25, out(0));

  in(0) = 50;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(35, out(0));

  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_RESET));
  in(0) = 8;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(8, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: float moving average reports the configured rolling window.
//***************************************************************************

static void test_prog_moving_avg_float_window()
{
  CDescObject srcDesc(g_cfg_movavg_src_f32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_movavg_dst_f32);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_movavg_f32);
  CProgMovingAverage prog(progDesc);
  io_sdata_t<float, 1, 1> in;
  io_sdata_t<float, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(MOVAVG_SRC_F32, &src);
  prog.setObjectMapItem(MOVAVG_DST_F32, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 1.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, out(0));

  in(0) = 2.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.5f, out(0));

  in(0) = 4.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 7.0f / 3.0f, out(0));

  in(0) = 7.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 13.0f / 3.0f, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: uint16 moving average uses the shared unsigned integer path.
//***************************************************************************

static void test_prog_moving_avg_uint16_window()
{
  CDescObject srcDesc(g_cfg_movavg_src_u16);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_movavg_dst_u16);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_movavg_u16);
  CProgMovingAverage prog(progDesc);
  io_sdata_t<uint16_t, 1, 1> in;
  io_sdata_t<uint16_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(MOVAVG_SRC_U16, &src);
  prog.setObjectMapItem(MOVAVG_DST_U16, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 10;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(10, out(0));

  in(0) = 20;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(15, out(0));

  in(0) = 40;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(30, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: double moving average uses the shared floating-point path.
//***************************************************************************

static void test_prog_moving_avg_double_window()
{
  CDescObject srcDesc(g_cfg_movavg_src_f64);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_movavg_dst_f64);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_movavg_f64);
  CProgMovingAverage prog(progDesc);
  io_sdata_t<double, 1, 1> in;
  io_sdata_t<double, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(MOVAVG_SRC_F64, &src);
  prog.setObjectMapItem(MOVAVG_DST_F64, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 1.0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001, 1.0, out(0));

  in(0) = 2.0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001, 1.5, out(0));

  in(0) = 4.0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001, 3.0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_moving_avg()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_prog_moving_avg_int32_window_and_reset);
    DAWN_RUN_TEST(test_prog_moving_avg_float_window);
    DAWN_RUN_TEST(test_prog_moving_avg_uint16_window);
    DAWN_RUN_TEST(test_prog_moving_avg_double_window);
    return UNITY_END();
  }
}
