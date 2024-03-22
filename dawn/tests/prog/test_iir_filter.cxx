// dawn/tests/prog/test_iir_filter.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/iirfilter.hxx"
#include "test_process_common.hxx"

using namespace dawn;

static constexpr auto IIR_SRC_I32 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 51);
static constexpr auto IIR_DST_I32 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 52);
static constexpr auto IIR_SRC_F32 = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 53);
static constexpr auto IIR_DST_F32 = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 54);
static constexpr auto IIR_SRC_U16 = CIOVirt::objectId(SObjectId::DTYPE_UINT16, false, 55);
static constexpr auto IIR_DST_U16 = CIOVirt::objectId(SObjectId::DTYPE_UINT16, false, 56);
static constexpr auto IIR_SRC_F64 = CIOVirt::objectId(SObjectId::DTYPE_DOUBLE, false, 57);
static constexpr auto IIR_DST_F64 = CIOVirt::objectId(SObjectId::DTYPE_DOUBLE, false, 58);

static uint32_t g_cfg_iir_src_i32[] = {
  IIR_SRC_I32,
  0,
};

static uint32_t g_cfg_iir_dst_i32[] = {
  IIR_DST_I32,
  0,
};

static uint32_t g_cfg_iir_src_f32[] = {
  IIR_SRC_F32,
  0,
};

static uint32_t g_cfg_iir_dst_f32[] = {
  IIR_DST_F32,
  0,
};

static uint32_t g_cfg_iir_src_u16[] = {
  IIR_SRC_U16,
  0,
};

static uint32_t g_cfg_iir_dst_u16[] = {
  IIR_DST_U16,
  0,
};

static uint32_t g_cfg_iir_src_f64[] = {
  IIR_SRC_F64,
  0,
};

static uint32_t g_cfg_iir_dst_f64[] = {
  IIR_DST_F64,
  0,
};

static uint32_t g_bin_iir_i32[] = {
  CProgIIRFilter::objectId(0),
  3,
  CProgIIRFilter::cfgIdIOBind(2),
  IIR_SRC_I32,
  IIR_DST_I32,
  CProgIIRFilter::cfgIdAlphaNum(),
  1,
  CProgIIRFilter::cfgIdAlphaDen(),
  2,
};

static uint32_t g_bin_iir_f32[] = {
  CProgIIRFilter::objectId(2),
  3,
  CProgIIRFilter::cfgIdIOBind(2),
  IIR_SRC_F32,
  IIR_DST_F32,
  CProgIIRFilter::cfgIdAlphaNum(),
  1,
  CProgIIRFilter::cfgIdAlphaDen(),
  4,
};

static uint32_t g_bin_iir_u16[] = {
  CProgIIRFilter::objectId(3),
  3,
  CProgIIRFilter::cfgIdIOBind(2),
  IIR_SRC_U16,
  IIR_DST_U16,
  CProgIIRFilter::cfgIdAlphaNum(),
  1,
  CProgIIRFilter::cfgIdAlphaDen(),
  4,
};

static uint32_t g_bin_iir_f64[] = {
  CProgIIRFilter::objectId(4),
  3,
  CProgIIRFilter::cfgIdIOBind(2),
  IIR_SRC_F64,
  IIR_DST_F64,
  CProgIIRFilter::cfgIdAlphaNum(),
  1,
  CProgIIRFilter::cfgIdAlphaDen(),
  2,
};

//***************************************************************************
// Description: int32 IIR filter applies alpha and reset clears history.
//***************************************************************************

static void test_prog_iir_filter_int32_and_reset()
{
  CDescObject srcDesc(g_cfg_iir_src_i32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_iir_dst_i32);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_iir_i32);
  CProgIIRFilter prog(progDesc);
  io_sdata_t<int32_t, 1, 1> in;
  io_sdata_t<int32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(IIR_SRC_I32, &src);
  prog.setObjectMapItem(IIR_DST_I32, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 10;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(10, out(0));

  in(0) = 14;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(12, out(0));

  in(0) = 18;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(15, out(0));

  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_RESET));
  in(0) = 4;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(4, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: float IIR filter applies the configured alpha ratio.
//***************************************************************************

static void test_prog_iir_filter_float_alpha()
{
  CDescObject srcDesc(g_cfg_iir_src_f32);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_iir_dst_f32);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_iir_f32);
  CProgIIRFilter prog(progDesc);
  io_sdata_t<float, 1, 1> in;
  io_sdata_t<float, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(IIR_SRC_F32, &src);
  prog.setObjectMapItem(IIR_DST_F32, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 8.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 8.0f, out(0));

  in(0) = 0.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 6.0f, out(0));

  in(0) = 0.0f;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001f, 4.5f, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: uint16 IIR filter uses the shared integer path.
//***************************************************************************

static void test_prog_iir_filter_uint16_alpha()
{
  CDescObject srcDesc(g_cfg_iir_src_u16);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_iir_dst_u16);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_iir_u16);
  CProgIIRFilter prog(progDesc);
  io_sdata_t<uint16_t, 1, 1> in;
  io_sdata_t<uint16_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(IIR_SRC_U16, &src);
  prog.setObjectMapItem(IIR_DST_U16, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 100;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(100, out(0));

  in(0) = 108;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(102, out(0));

  in(0) = 110;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(104, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: double IIR filter uses the shared floating-point path.
//***************************************************************************

static void test_prog_iir_filter_double_alpha()
{
  CDescObject srcDesc(g_cfg_iir_src_f64);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_iir_dst_f64);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_iir_f64);
  CProgIIRFilter prog(progDesc);
  io_sdata_t<double, 1, 1> in;
  io_sdata_t<double, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(IIR_SRC_F64, &src);
  prog.setObjectMapItem(IIR_DST_F64, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  in(0) = 2.0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001, 2.0, out(0));

  in(0) = 4.0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_FLOAT_WITHIN(0.001, 3.0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_iir_filter()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_prog_iir_filter_int32_and_reset);
    DAWN_RUN_TEST(test_prog_iir_filter_float_alpha);
    DAWN_RUN_TEST(test_prog_iir_filter_uint16_alpha);
    DAWN_RUN_TEST(test_prog_iir_filter_double_alpha);
    return UNITY_END();
  }
}
