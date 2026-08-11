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

// min 0, max 4095: the 12-bit SAADC window, with min stored as a two's
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

extern "C"
{
  int test_prog_saturate()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_saturate_clamps_scalar);
    DAWN_RUN_TEST(test_saturate_open_upper_bound);
    DAWN_RUN_TEST(test_saturate_clamps_batch);
    DAWN_RUN_TEST(test_saturate_narrows_int32_to_int16);
    return UNITY_END();
  }
}
