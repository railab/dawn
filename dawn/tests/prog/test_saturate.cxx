// dawn/tests/prog/test_saturate.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/saturate.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto SAT_I16_I = CIOVirt::objectId(SObjectId::DTYPE_INT16, false, 130);
static constexpr auto SAT_I16_O = CIOVirt::objectId(SObjectId::DTYPE_INT16, false, 131);

static uint32_t g_i16_i[] = {
  SAT_I16_I,
  0,
};

static uint32_t g_i16_o[] = {
  SAT_I16_O,
  0,
};

// min 0, max 4095: the 12-bit ADC window, with min stored as a two's
// complement word exactly as the descriptor holds it.

static uint32_t g_sat_bin[] = {
  CProgSaturate::objectId(0),
  4,
  CProgSaturate::cfgIdInput(),
  SAT_I16_I,
  CProgSaturate::cfgIdOutput(),
  SAT_I16_O,
  CProgSaturate::cfgIdMin(),
  0,
  CProgSaturate::cfgIdMax(),
  4095,
};

// Only a lower bound: the upper side stays at the data type limit.

static uint32_t g_sat_lo_bin[] = {
  CProgSaturate::objectId(1),
  3,
  CProgSaturate::cfgIdInput(),
  SAT_I16_I,
  CProgSaturate::cfgIdOutput(),
  SAT_I16_O,
  CProgSaturate::cfgIdMin(),
  static_cast<uint32_t>(-100),
};

static constexpr auto SAT_I32_I = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 132);

static uint32_t g_i32_i[] = {
  SAT_I32_I,
  0,
};

// int32 in, int16 out: the clamp is what makes the narrowing store safe.

static uint32_t g_sat_narrow_bin[] = {
  CProgSaturate::objectId(2),
  4,
  CProgSaturate::cfgIdInput(),
  SAT_I32_I,
  CProgSaturate::cfgIdOutput(),
  SAT_I16_O,
  CProgSaturate::cfgIdMin(),
  0,
  CProgSaturate::cfgIdMax(),
  4095,
};

static constexpr auto SAT_I16_I2 = CIOVirt::objectId(SObjectId::DTYPE_INT16, false, 133);

static uint32_t g_i16_i2[] = {
  SAT_I16_I2,
  0,
};

// Second limiter on the same output, fed from a differently batched input.

static uint32_t g_sat_shared_bin[] = {
  CProgSaturate::objectId(3),
  4,
  CProgSaturate::cfgIdInput(),
  SAT_I16_I2,
  CProgSaturate::cfgIdOutput(),
  SAT_I16_O,
  CProgSaturate::cfgIdMin(),
  0,
  CProgSaturate::cfgIdMax(),
  4095,
};

// Runtime-writable bounds.

static uint32_t g_sat_rw_bin[] = {
  CProgSaturate::objectId(4),
  4,
  CProgSaturate::cfgIdInput(),
  SAT_I16_I,
  CProgSaturate::cfgIdOutput(),
  SAT_I16_O,
  CProgSaturate::cfgIdMin(true),
  0,
  CProgSaturate::cfgIdMax(true),
  4095,
};

// max does not fit int16.

static uint32_t g_sat_range_bin[] = {
  CProgSaturate::objectId(5),
  3,
  CProgSaturate::cfgIdInput(),
  SAT_I16_I,
  CProgSaturate::cfgIdOutput(),
  SAT_I16_O,
  CProgSaturate::cfgIdMax(),
  40000,
};

// min above max.

static uint32_t g_sat_inverted_bin[] = {
  CProgSaturate::objectId(6),
  4,
  CProgSaturate::cfgIdInput(),
  SAT_I16_I,
  CProgSaturate::cfgIdOutput(),
  SAT_I16_O,
  CProgSaturate::cfgIdMin(),
  10,
  CProgSaturate::cfgIdMax(),
  5,
};

#ifdef CONFIG_DAWN_DTYPE_FLOAT
static constexpr auto SAT_F32_I = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 134);
static constexpr auto SAT_F32_O = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 135);

static uint32_t g_f32_i[] = {
  SAT_F32_I,
  0,
};

static uint32_t g_f32_o[] = {
  SAT_F32_O,
  0,
};

// Real bounds are float words.

static uint32_t g_sat_f32_bin[] = {
  CProgSaturate::objectId(7),
  4,
  CProgSaturate::cfgIdInput(),
  SAT_F32_I,
  CProgSaturate::cfgIdOutput(),
  SAT_F32_O,
  CProgSaturate::cfgIdMin(),
  SObjectCfg::fToCfg(0.0f),
  CProgSaturate::cfgIdMax(),
  SObjectCfg::fToCfg(1.0f),
};

// float in, int16 out, no explicit bounds: the int16 range applies.

static uint32_t g_sat_f32_i16_bin[] = {
  CProgSaturate::objectId(8),
  3,
  CProgSaturate::cfgIdInput(),
  SAT_F32_I,
  CProgSaturate::cfgIdOutput(),
  SAT_I16_O,
  CProgSaturate::cfgIdMin(),
  SObjectCfg::fToCfg(-1000.0f),
};
#endif

static io_ddata_t *g_notified;

static int satNotifyCb(void *priv, io_ddata_t *data)
{
  UNUSED(priv);
  g_notified = data;
  return OK;
}

//***************************************************************************
// Description: saturate clamps a scalar input into the configured range and
// passes in-range values through untouched.
//***************************************************************************

static void test_saturate_clamps_scalar()
{
  CDescObject id(g_i16_i);
  CIOVirt in(id);
  CDescObject od(g_i16_o);
  CIOVirt out(od);
  CDescObject pd(g_sat_bin);
  CProgSaturate p(pd);
  io_sdata_t<int16_t, 1, 1> inData;
  io_sdata_t<int16_t, 1, 1> outData;

  TEST_ASSERT_EQUAL(OK, in.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SAT_I16_I, &in);
  p.setObjectMapItem(SAT_I16_O, &out);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  inData(0) = -5;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(0, outData(0));

  inData(0) = 5000;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(4095, outData(0));

  inData(0) = 1234;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(1234, outData(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: an omitted max leaves the upper side at the data type limit,
// and a negative min round-trips through its descriptor word.
//***************************************************************************

static void test_saturate_open_upper_bound()
{
  CDescObject id(g_i16_i);
  CIOVirt in(id);
  CDescObject od(g_i16_o);
  CIOVirt out(od);
  CDescObject pd(g_sat_lo_bin);
  CProgSaturate p(pd);
  io_sdata_t<int16_t, 1, 1> inData;
  io_sdata_t<int16_t, 1, 1> outData;

  TEST_ASSERT_EQUAL(OK, in.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SAT_I16_I, &in);
  p.setObjectMapItem(SAT_I16_O, &out);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  inData(0) = -200;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(-100, outData(0));

  inData(0) = 32767;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(32767, outData(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: a batched input is clamped sample by sample and republished
// with its batch intact.
//***************************************************************************

static void test_saturate_clamps_batch()
{
  CDescObject id(g_i16_i);
  CIOVirt in(id);
  CDescObject od(g_i16_o);
  CIOVirt out(od);
  CDescObject pd(g_sat_bin);
  CProgSaturate p(pd);
  io_sdata_t<int16_t, 1, 4> inData;
  int16_t *o;

  TEST_ASSERT_EQUAL(OK, in.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in.initialize(1, 4, true));

  // The batch of a virt IO comes from initialize(), so a downstream program
  // has to see it there.

  TEST_ASSERT_TRUE(in.isBatch());
  TEST_ASSERT_EQUAL(4, in.getNotifyBatch());

  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SAT_I16_I, &in);
  p.setObjectMapItem(SAT_I16_O, &out);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  g_notified = nullptr;
  TEST_ASSERT_EQUAL(OK, out.setNotifier(satNotifyCb, 0, nullptr));

  inData(0, 0) = -1;
  inData(0, 1) = 0;
  inData(0, 2) = 4095;
  inData(0, 3) = 4096;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));

  TEST_ASSERT_NOT_NULL(g_notified);
  TEST_ASSERT_EQUAL(4, g_notified->getBatch());

  o = static_cast<int16_t *>(g_notified->getDataPtr(0));
  TEST_ASSERT_EQUAL(0, o[0]);
  o = static_cast<int16_t *>(g_notified->getDataPtr(1));
  TEST_ASSERT_EQUAL(0, o[0]);
  o = static_cast<int16_t *>(g_notified->getDataPtr(2));
  TEST_ASSERT_EQUAL(4095, o[0]);
  o = static_cast<int16_t *>(g_notified->getDataPtr(3));
  TEST_ASSERT_EQUAL(4095, o[0]);

  out.setNotifier(nullptr, 0, nullptr);
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: an int32 input is clamped and stored into an int16 output,
// so no separate conversion step is needed.
//***************************************************************************

static void test_saturate_narrows_int32_to_int16()
{
  CDescObject id(g_i32_i);
  CIOVirt in(id);
  CDescObject od(g_i16_o);
  CIOVirt out(od);
  CDescObject pd(g_sat_narrow_bin);
  CProgSaturate p(pd);
  io_sdata_t<int32_t, 1, 4> inData;
  int16_t *o;

  TEST_ASSERT_EQUAL(OK, in.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in.initialize(1, 4, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SAT_I32_I, &in);
  p.setObjectMapItem(SAT_I16_O, &out);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  g_notified = nullptr;
  TEST_ASSERT_EQUAL(OK, out.setNotifier(satNotifyCb, 0, nullptr));

  inData(0, 0) = -1;
  inData(0, 1) = 1234;
  inData(0, 2) = 4095;
  inData(0, 3) = 100000;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));

  TEST_ASSERT_NOT_NULL(g_notified);
  TEST_ASSERT_EQUAL(2, g_notified->getSize());

  o = static_cast<int16_t *>(g_notified->getDataPtr(0));
  TEST_ASSERT_EQUAL(0, o[0]);
  o = static_cast<int16_t *>(g_notified->getDataPtr(1));
  TEST_ASSERT_EQUAL(1234, o[0]);
  o = static_cast<int16_t *>(g_notified->getDataPtr(2));
  TEST_ASSERT_EQUAL(4095, o[0]);
  o = static_cast<int16_t *>(g_notified->getDataPtr(3));
  TEST_ASSERT_EQUAL(4095, o[0]);

  out.setNotifier(nullptr, 0, nullptr);
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: a virt output already prepared with one batch rejects a
// second writer that expects another batch instead of silently decimating.
//***************************************************************************

static void test_saturate_rejects_batch_mismatch_on_shared_output()
{
  CDescObject id1(g_i16_i);
  CIOVirt in1(id1);
  CDescObject id2(g_i16_i2);
  CIOVirt in2(id2);
  CDescObject od(g_i16_o);
  CIOVirt out(od);
  CDescObject pd1(g_sat_bin);
  CProgSaturate p1(pd1);
  CDescObject pd2(g_sat_shared_bin);
  CProgSaturate p2(pd2);

  TEST_ASSERT_EQUAL(OK, in1.init());
  TEST_ASSERT_EQUAL(OK, in2.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in1.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, in2.initialize(1, 4, true));

  TEST_ASSERT_EQUAL(OK, p1.configure());
  p1.setObjectMapItem(SAT_I16_I, &in1);
  p1.setObjectMapItem(SAT_I16_O, &out);
  TEST_ASSERT_EQUAL(OK, p1.init());
  TEST_ASSERT_EQUAL(1, out.getNotifyBatch());

  TEST_ASSERT_EQUAL(OK, p2.configure());
  p2.setObjectMapItem(SAT_I16_I2, &in2);
  p2.setObjectMapItem(SAT_I16_O, &out);
  TEST_ASSERT_EQUAL(-EINVAL, p2.init());
}

//***************************************************************************
// Description: rw bounds take effect on the next sample via setObjConfig,
// and a write that would invert the pair is rejected without touching the
// active bounds.
//***************************************************************************

static void test_saturate_runtime_bounds_update_and_rollback()
{
  CDescObject id(g_i16_i);
  CIOVirt in(id);
  CDescObject od(g_i16_o);
  CIOVirt out(od);
  CDescObject pd(g_sat_rw_bin);
  CProgSaturate p(pd);
  io_sdata_t<int16_t, 1, 1> inData;
  io_sdata_t<int16_t, 1, 1> outData;
  uint32_t v;

  TEST_ASSERT_EQUAL(OK, in.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SAT_I16_I, &in);
  p.setObjectMapItem(SAT_I16_O, &out);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  v = 100;
  TEST_ASSERT_EQUAL(OK, p.setObjConfig(CProgSaturate::cfgIdMax(true), &v, 1));

  inData(0) = 5000;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(100, outData(0));

  // min above the current max: rejected, old pair stays active

  v = 200;
  TEST_ASSERT_EQUAL(-EINVAL, p.setObjConfig(CProgSaturate::cfgIdMin(true), &v, 1));

  inData(0) = -5;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(0, outData(0));

  inData(0) = 5000;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(100, outData(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

// Configure and bind in -> out; returns p.init().

static int saturateInit(uint32_t *bin, CIOVirt &in, CIOVirt &out)
{
  CDescObject pd(bin);
  CProgSaturate p(pd);

  in.init();
  out.init();
  in.initialize(1, 1, true);
  p.configure();
  p.setObjectMapItem(in.getIdV(), &in);
  p.setObjectMapItem(out.getIdV(), &out);
  return p.init();
}

//***************************************************************************
// Description: a bound outside the data type range is rejected at init.
//***************************************************************************

static void test_saturate_rejects_bound_outside_range()
{
  CDescObject id(g_i16_i);
  CIOVirt in(id);
  CDescObject od(g_i16_o);
  CIOVirt out(od);

  TEST_ASSERT_EQUAL(-ERANGE, saturateInit(g_sat_range_bin, in, out));
}

//***************************************************************************
// Description: min above max is rejected at init.
//***************************************************************************

static void test_saturate_rejects_min_above_max()
{
  CDescObject id(g_i16_i);
  CIOVirt in(id);
  CDescObject od(g_i16_o);
  CIOVirt out(od);

  TEST_ASSERT_EQUAL(-EINVAL, saturateInit(g_sat_inverted_bin, in, out));
}

//***************************************************************************
// Description: a fetch-only input is read and clamped once at start.
//***************************************************************************

static void test_saturate_fetch_only_input()
{
  CDescObject id(g_i16_i);
  CIOVirt in(id);
  CDescObject od(g_i16_o);
  CIOVirt out(od);
  CDescObject pd(g_sat_bin);
  CProgSaturate p(pd);
  io_sdata_t<int16_t, 1, 1> inData;
  io_sdata_t<int16_t, 1, 1> outData;

  TEST_ASSERT_EQUAL(OK, in.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in.initialize(1, 1, false));
  TEST_ASSERT_FALSE(in.isNotify());
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SAT_I16_I, &in);
  p.setObjectMapItem(SAT_I16_O, &out);
  TEST_ASSERT_EQUAL(OK, p.init());

  inData(0) = 5000;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));

  TEST_ASSERT_EQUAL(OK, p.start());
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(4095, outData(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

#ifdef CONFIG_DAWN_DTYPE_FLOAT
//***************************************************************************
// Description: float samples are clamped against float-word bounds.
//***************************************************************************

static void test_saturate_clamps_float()
{
  CDescObject id(g_f32_i);
  CIOVirt in(id);
  CDescObject od(g_f32_o);
  CIOVirt out(od);
  CDescObject pd(g_sat_f32_bin);
  CProgSaturate p(pd);
  io_sdata_t<float, 1, 1> inData;
  io_sdata_t<float, 1, 1> outData;

  TEST_ASSERT_EQUAL(OK, in.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SAT_F32_I, &in);
  p.setObjectMapItem(SAT_F32_O, &out);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  inData(0) = -0.5f;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL_FLOAT(0.0f, outData(0));

  inData(0) = 2.5f;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL_FLOAT(1.0f, outData(0));

  inData(0) = 0.25f;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL_FLOAT(0.25f, outData(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: a float input stored into int16 is clamped to what int16 can
// hold, so the narrowing store cannot overflow.
//***************************************************************************

static void test_saturate_float_to_int16()
{
  CDescObject id(g_f32_i);
  CIOVirt in(id);
  CDescObject od(g_i16_o);
  CIOVirt out(od);
  CDescObject pd(g_sat_f32_i16_bin);
  CProgSaturate p(pd);
  io_sdata_t<float, 1, 1> inData;
  io_sdata_t<int16_t, 1, 1> outData;

  TEST_ASSERT_EQUAL(OK, in.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SAT_F32_I, &in);
  p.setObjectMapItem(SAT_I16_O, &out);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  inData(0) = 1.0e6f;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(32767, outData(0));

  inData(0) = -1.0e6f;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(-1000, outData(0));

  inData(0) = 12.75f;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL(12, outData(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}
#endif

extern "C"
{
  int test_prog_saturate()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_saturate_clamps_scalar);
    DAWN_RUN_TEST(test_saturate_open_upper_bound);
    DAWN_RUN_TEST(test_saturate_clamps_batch);
    DAWN_RUN_TEST(test_saturate_narrows_int32_to_int16);
    DAWN_RUN_TEST(test_saturate_rejects_batch_mismatch_on_shared_output);
    DAWN_RUN_TEST(test_saturate_runtime_bounds_update_and_rollback);
    DAWN_RUN_TEST(test_saturate_rejects_bound_outside_range);
    DAWN_RUN_TEST(test_saturate_rejects_min_above_max);
    DAWN_RUN_TEST(test_saturate_fetch_only_input);
#ifdef CONFIG_DAWN_DTYPE_FLOAT
    DAWN_RUN_TEST(test_saturate_clamps_float);
    DAWN_RUN_TEST(test_saturate_float_to_int16);
#endif
    return UNITY_END();
  }
}
