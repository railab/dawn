// dawn/tests/prog/test_threshold.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/threshold.hxx"
#include "test_process_common.hxx"

using namespace dawn;

static constexpr auto THRESH_SRC_I32 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 51);
static constexpr auto THRESH_DST_BOOL = CIOVirt::objectId(SObjectId::DTYPE_BOOL, false, 52);
static constexpr auto THRESH_SRC_F32 = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 53);
static constexpr auto THRESH_DST_F32_BOOL = CIOVirt::objectId(SObjectId::DTYPE_BOOL, false, 54);
static constexpr auto THRESH_DST_I32 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 55);
static constexpr auto THRESH_DST_F32 = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 56);
static constexpr auto THRESH_SRC_U16 = CIOVirt::objectId(SObjectId::DTYPE_UINT16, false, 57);
static constexpr auto THRESH_DST_U16 = CIOVirt::objectId(SObjectId::DTYPE_UINT16, false, 58);

static uint32_t g_cfg_thresh_src_i32[] = {
  THRESH_SRC_I32,
  0,
};

static uint32_t g_cfg_thresh_dst_bool[] = {
  THRESH_DST_BOOL,
  0,
};

static uint32_t g_cfg_thresh_src_f32[] = {
  THRESH_SRC_F32,
  0,
};

static uint32_t g_cfg_thresh_dst_f32_bool[] = {
  THRESH_DST_F32_BOOL,
  0,
};

static uint32_t g_cfg_thresh_dst_i32[] = {
  THRESH_DST_I32,
  0,
};

static uint32_t g_cfg_thresh_dst_f32[] = {
  THRESH_DST_F32,
  0,
};

static uint32_t g_cfg_thresh_src_u16[] = {
  THRESH_SRC_U16,
  0,
};

static uint32_t g_cfg_thresh_dst_u16[] = {
  THRESH_DST_U16,
  0,
};

static uint32_t g_bin_threshold_above[] = {
  CProgThreshold::objectId(0),
  4,
  CProgThreshold::cfgIdIOBind(2),
  THRESH_SRC_I32,
  THRESH_DST_BOOL,
  CProgThreshold::cfgIdMode(),
  CProgThreshold::MODE_ABOVE,
  CProgThreshold::cfgIdHigh(),
  10,
  CProgThreshold::cfgIdLow(),
  0,
};

static uint32_t g_bin_threshold_hyst[] = {
  CProgThreshold::objectId(2),
  4,
  CProgThreshold::cfgIdIOBind(2),
  THRESH_SRC_I32,
  THRESH_DST_BOOL,
  CProgThreshold::cfgIdMode(),
  CProgThreshold::MODE_HYSTERESIS,
  CProgThreshold::cfgIdLow(),
  8,
  CProgThreshold::cfgIdHigh(),
  12,
};

static uint32_t g_bin_threshold_window_float[] = {
  CProgThreshold::objectId(3),
  4,
  CProgThreshold::cfgIdIOBind(2),
  THRESH_SRC_F32,
  THRESH_DST_F32_BOOL,
  CProgThreshold::cfgIdMode(),
  CProgThreshold::MODE_WINDOW,
  CProgThreshold::cfgIdLow(),
  SObjectCfg::fToCfg(2.0f),
  CProgThreshold::cfgIdHigh(),
  SObjectCfg::fToCfg(5.0f),
};

static uint32_t g_bin_threshold_value_above[] = {
  CProgThresholdValue::objectId(0),
  4,
  CProgThresholdValue::cfgIdIOBind(2),
  THRESH_SRC_I32,
  THRESH_DST_I32,
  CProgThresholdValue::cfgIdMode(),
  CProgThresholdValue::MODE_ABOVE,
  CProgThresholdValue::cfgIdLow(),
  0,
  CProgThresholdValue::cfgIdHigh(),
  10,
};

static uint32_t g_bin_threshold_value_hyst_float[] = {
  CProgThresholdValue::objectId(2),
  4,
  CProgThresholdValue::cfgIdIOBind(2),
  THRESH_SRC_F32,
  THRESH_DST_F32,
  CProgThresholdValue::cfgIdMode(),
  CProgThresholdValue::MODE_HYSTERESIS,
  CProgThresholdValue::cfgIdLow(),
  SObjectCfg::fToCfg(2.0f),
  CProgThresholdValue::cfgIdHigh(),
  SObjectCfg::fToCfg(4.0f),
};

static uint32_t g_bin_threshold_value_window_u16[] = {
  CProgThresholdValue::objectId(3),
  4,
  CProgThresholdValue::cfgIdIOBind(2),
  THRESH_SRC_U16,
  THRESH_DST_U16,
  CProgThresholdValue::cfgIdMode(),
  CProgThresholdValue::MODE_WINDOW,
  CProgThresholdValue::cfgIdLow(),
  4,
  CProgThresholdValue::cfgIdHigh(),
  8,
};

//***************************************************************************
// Description: above-threshold mode writes true at and above the high limit.
//***************************************************************************

static void test_prog_threshold_above_int32()
{
  CDescObject srcDesc(g_cfg_thresh_src_i32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_thresh_dst_bool);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_threshold_above);
  CProgThreshold prog(progDesc);
  io_sdata_t<int32_t, 1, 1> in;
  io_sdata_t<uint8_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(THRESH_SRC_I32, &src);
  prog.setObjectMapItem(THRESH_DST_BOOL, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 9;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  in(0) = 10;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  in(0) = -1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: hysteresis mode latches between low/high limits and resets.
//***************************************************************************

static void test_prog_threshold_hysteresis_and_reset()
{
  CDescObject srcDesc(g_cfg_thresh_src_i32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_thresh_dst_bool);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_threshold_hyst);
  CProgThreshold prog(progDesc);
  io_sdata_t<int32_t, 1, 1> in;
  io_sdata_t<uint8_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(THRESH_SRC_I32, &src);
  prog.setObjectMapItem(THRESH_DST_BOOL, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 7;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  in(0) = 13;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  in(0) = 10;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  in(0) = 8;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_RESET));
  in(0) = 10;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: float window mode is true only inside the configured range.
//***************************************************************************

static void test_prog_threshold_window_float()
{
  CDescObject srcDesc(g_cfg_thresh_src_f32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_thresh_dst_f32_bool);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_threshold_window_float);
  CProgThreshold prog(progDesc);
  io_sdata_t<float, 1, 1> in;
  io_sdata_t<uint8_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(THRESH_SRC_F32, &src);
  prog.setObjectMapItem(THRESH_DST_F32_BOOL, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 1.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  in(0) = 3.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  in(0) = 6.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: threshold-value mode forwards source values above the limit.
//***************************************************************************

static void test_prog_threshold_value_above_int32()
{
  CDescObject srcDesc(g_cfg_thresh_src_i32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_thresh_dst_i32);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_threshold_value_above);
  CProgThresholdValue prog(progDesc);
  io_sdata_t<int32_t, 1, 1> in;
  io_sdata_t<int32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(THRESH_SRC_I32, &src);
  prog.setObjectMapItem(THRESH_DST_I32, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 9;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  in(0) = 11;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(11, out(0));

  in(0) = -3;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: float threshold-value hysteresis forwards values until reset.
//***************************************************************************

static void test_prog_threshold_value_hysteresis_float_reset()
{
  CDescObject srcDesc(g_cfg_thresh_src_f32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_thresh_dst_f32);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_threshold_value_hyst_float);
  CProgThresholdValue prog(progDesc);
  io_sdata_t<float, 1, 1> in;
  io_sdata_t<float, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(THRESH_SRC_F32, &src);
  prog.setObjectMapItem(THRESH_DST_F32, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 4.5f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.5f, out(0));

  in(0) = 3.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 3.0f, out(0));

  in(0) = 1.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out(0));

  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_RESET));
  in(0) = 3.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 0.0f, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: threshold-value supports uint16 source/output conversion path.
//***************************************************************************

static void test_prog_threshold_value_window_uint16()
{
  CDescObject srcDesc(g_cfg_thresh_src_u16);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_thresh_dst_u16);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_threshold_value_window_u16);
  CProgThresholdValue prog(progDesc);
  io_sdata_t<uint16_t, 1, 1> in;
  io_sdata_t<uint16_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(THRESH_SRC_U16, &src);
  prog.setObjectMapItem(THRESH_DST_U16, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 3;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  in(0) = 6;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(6, out(0));

  in(0) = 9;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_threshold()
  {
    UNITY_BEGIN();
#ifdef CONFIG_DAWN_PROG_THRESHOLD
    DAWN_RUN_TEST(test_prog_threshold_above_int32);
    DAWN_RUN_TEST(test_prog_threshold_hysteresis_and_reset);
    DAWN_RUN_TEST(test_prog_threshold_window_float);
#endif
#ifdef CONFIG_DAWN_PROG_THRESHOLD_VALUE
    DAWN_RUN_TEST(test_prog_threshold_value_above_int32);
    DAWN_RUN_TEST(test_prog_threshold_value_hysteresis_float_reset);
    DAWN_RUN_TEST(test_prog_threshold_value_window_uint16);
#endif
    return UNITY_END();
  }
}
