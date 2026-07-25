// dawn/tests/prog/test_bitmerge.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/bitmerge.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto BM_I16_S = CIOVirt::objectId(SObjectId::DTYPE_INT16, false, 140);
static constexpr auto BM_I16_T = CIOVirt::objectId(SObjectId::DTYPE_INT16, false, 141);
static constexpr auto BM_I16_O = CIOVirt::objectId(SObjectId::DTYPE_INT16, false, 142);

static uint32_t g_i16_s[] = {
  BM_I16_S,
  0,
};

static uint32_t g_i16_t[] = {
  BM_I16_T,
  0,
};

static uint32_t g_i16_o[] = {
  BM_I16_O,
  0,
};

// 12-bit samples tagged with a 3-bit value at bit 12: fits an int16 word.

static uint32_t g_fit_bin[] = {
  CProgBitMerge::objectId(0),
  2,
  CProgBitMerge::cfgIdInputs(6),
  BM_I16_S,
  0,
  0xfff,
  BM_I16_T,
  12,
  0x7,
  CProgBitMerge::cfgIdOutput(),
  BM_I16_O,
};

// The same tag at bit 14 fits a 32-bit word but overflows the int16 output.

static uint32_t g_overflow_bin[] = {
  CProgBitMerge::objectId(1),
  2,
  CProgBitMerge::cfgIdInputs(6),
  BM_I16_S,
  0,
  0xfff,
  BM_I16_T,
  14,
  0x7,
  CProgBitMerge::cfgIdOutput(),
  BM_I16_O,
};

static constexpr auto BM_U64_T = CIOVirt::objectId(SObjectId::DTYPE_UINT64, false, 143);
static constexpr auto BM_I16_TS = CIOVirt::objectId(SObjectId::DTYPE_INT16, true, 144);

static uint32_t g_u64_t[] = {
  BM_U64_T,
  0,
};

static uint32_t g_i16_ts[] = {
  BM_I16_TS,
  0,
};

// An empty mask contributes nothing and is a descriptor mistake.

static uint32_t g_empty_mask_bin[] = {
  CProgBitMerge::objectId(2),
  2,
  CProgBitMerge::cfgIdInputs(6),
  BM_I16_S,
  0,
  0xfff,
  BM_I16_T,
  12,
  0x0,
  CProgBitMerge::cfgIdOutput(),
  BM_I16_O,
};

// Bit 11 is claimed by both fields.

static uint32_t g_overlap_bin[] = {
  CProgBitMerge::objectId(3),
  2,
  CProgBitMerge::cfgIdInputs(6),
  BM_I16_S,
  0,
  0xfff,
  BM_I16_T,
  11,
  0x7,
  CProgBitMerge::cfgIdOutput(),
  BM_I16_O,
};

// A 64-bit tag input cannot be merged in a 32-bit word.

static uint32_t g_u64_bin[] = {
  CProgBitMerge::objectId(4),
  2,
  CProgBitMerge::cfgIdInputs(6),
  BM_I16_S,
  0,
  0xfff,
  BM_U64_T,
  12,
  0x7,
  CProgBitMerge::cfgIdOutput(),
  BM_I16_O,
};

// Same layout as g_fit_bin, streamed from a timestamped input.

static uint32_t g_ts_bin[] = {
  CProgBitMerge::objectId(5),
  2,
  CProgBitMerge::cfgIdInputs(6),
  BM_I16_TS,
  0,
  0xfff,
  BM_I16_T,
  12,
  0x7,
  CProgBitMerge::cfgIdOutput(),
  BM_I16_O,
};

static io_ddata_t *g_notified;

static int bmNotifyCb(void *priv, io_ddata_t *data)
{
  UNUSED(priv);
  g_notified = data;
  return OK;
}

//***************************************************************************
// Description: a batched input is tagged sample by sample with the slow
// input folded into the configured field.
//***************************************************************************

static void test_bitmerge_tags_batch()
{
  CDescObject sd(g_i16_s);
  CIOVirt s(sd);
  CDescObject td(g_i16_t);
  CIOVirt t(td);
  CDescObject od(g_i16_o);
  CIOVirt o(od);
  CDescObject pd(g_fit_bin);
  CProgBitMerge p(pd);
  io_sdata_t<int16_t, 1, 4> sData;
  io_sdata_t<int16_t, 1, 1> tData;
  int16_t *out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, o.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 4, true));
  TEST_ASSERT_EQUAL(OK, t.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BM_I16_S, &s);
  p.setObjectMapItem(BM_I16_T, &t);
  p.setObjectMapItem(BM_I16_O, &o);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  g_notified = nullptr;
  TEST_ASSERT_EQUAL(OK, o.setNotifier(bmNotifyCb, 0, nullptr));

  tData(0) = 5;
  TEST_ASSERT_EQUAL(OK, t.setData(tData));

  sData(0, 0) = 0;
  sData(0, 1) = 1;
  sData(0, 2) = 0xfff;
  sData(0, 3) = 0x1234;
  TEST_ASSERT_EQUAL(OK, s.setData(sData));

  TEST_ASSERT_NOT_NULL(g_notified);
  TEST_ASSERT_EQUAL(4, g_notified->getBatch());

  out = static_cast<int16_t *>(g_notified->getDataPtr(0));
  TEST_ASSERT_EQUAL(0x5000, out[0]);
  out = static_cast<int16_t *>(g_notified->getDataPtr(1));
  TEST_ASSERT_EQUAL(0x5001, out[0]);
  out = static_cast<int16_t *>(g_notified->getDataPtr(2));
  TEST_ASSERT_EQUAL(0x5fff, out[0]);
  out = static_cast<int16_t *>(g_notified->getDataPtr(3));
  TEST_ASSERT_EQUAL(0x5234, out[0]);

  o.setNotifier(nullptr, 0, nullptr);
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: a field that fits a 32-bit word but not the narrower output
// element is rejected at init instead of being silently truncated.
//***************************************************************************

static void test_bitmerge_rejects_field_past_output_width()
{
  CDescObject sd(g_i16_s);
  CIOVirt s(sd);
  CDescObject td(g_i16_t);
  CIOVirt t(td);
  CDescObject od(g_i16_o);
  CIOVirt o(od);
  CDescObject pd(g_overflow_bin);
  CProgBitMerge p(pd);

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, o.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 4, true));
  TEST_ASSERT_EQUAL(OK, t.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BM_I16_S, &s);
  p.setObjectMapItem(BM_I16_T, &t);
  p.setObjectMapItem(BM_I16_O, &o);

  TEST_ASSERT_EQUAL(-EINVAL, p.init());
}

//***************************************************************************
// Description: with no batched input every change of a slow input republishes
// the merged scalar word.
//***************************************************************************

static void test_bitmerge_merges_scalar_inputs()
{
  CDescObject sd(g_i16_s);
  CIOVirt s(sd);
  CDescObject td(g_i16_t);
  CIOVirt t(td);
  CDescObject od(g_i16_o);
  CIOVirt o(od);
  CDescObject pd(g_fit_bin);
  CProgBitMerge p(pd);
  io_sdata_t<int16_t, 1, 1> in;
  io_sdata_t<int16_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, o.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, t.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BM_I16_S, &s);
  p.setObjectMapItem(BM_I16_T, &t);
  p.setObjectMapItem(BM_I16_O, &o);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  in(0) = 5;
  TEST_ASSERT_EQUAL(OK, t.setData(in));
  in(0) = 0x123;
  TEST_ASSERT_EQUAL(OK, s.setData(in));

  TEST_ASSERT_EQUAL(OK, o.getData(out, 1));
  TEST_ASSERT_EQUAL(0x5123, out(0));

  in(0) = 2;
  TEST_ASSERT_EQUAL(OK, t.setData(in));
  TEST_ASSERT_EQUAL(OK, o.getData(out, 1));
  TEST_ASSERT_EQUAL(0x2123, out(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: a multi-element slow input is accepted and only its element
// zero enters the merge (a bitpack output packs one 32-bit field per GPI).
//***************************************************************************

static void test_bitmerge_slow_input_uses_element_zero()
{
  CDescObject sd(g_i16_s);
  CIOVirt s(sd);
  CDescObject td(g_i16_t);
  CIOVirt t(td);
  CDescObject od(g_i16_o);
  CIOVirt o(od);
  CDescObject pd(g_fit_bin);
  CProgBitMerge p(pd);
  io_sdata_t<int16_t, 1, 1> in;
  io_sdata_t<int16_t, 2, 1> tag;
  io_sdata_t<int16_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, o.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, t.initialize(2, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BM_I16_S, &s);
  p.setObjectMapItem(BM_I16_T, &t);
  p.setObjectMapItem(BM_I16_O, &o);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  tag(0) = 3;
  tag(1) = 7;
  TEST_ASSERT_EQUAL(OK, t.setData(tag));
  in(0) = 0x123;
  TEST_ASSERT_EQUAL(OK, s.setData(in));

  TEST_ASSERT_EQUAL(OK, o.getData(out, 1));
  TEST_ASSERT_EQUAL(0x3123, out(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

// Configure and bind a scalar (s, t) -> o merge; returns p.init().

static int bitmergeInitScalar(uint32_t *bin, CIOVirt &s, CIOVirt &t, CIOVirt &o)
{
  CDescObject pd(bin);
  CProgBitMerge p(pd);

  s.init();
  t.init();
  o.init();
  s.initialize(1, 1, true);
  t.initialize(1, 1, true);
  p.configure();
  p.setObjectMapItem(s.getIdV(), &s);
  p.setObjectMapItem(t.getIdV(), &t);
  p.setObjectMapItem(o.getIdV(), &o);
  return p.init();
}

//***************************************************************************
// Description: an input with an empty mask is rejected at init.
//***************************************************************************

static void test_bitmerge_rejects_empty_mask()
{
  CDescObject sd(g_i16_s);
  CIOVirt s(sd);
  CDescObject td(g_i16_t);
  CIOVirt t(td);
  CDescObject od(g_i16_o);
  CIOVirt o(od);

  TEST_ASSERT_EQUAL(-EINVAL, bitmergeInitScalar(g_empty_mask_bin, s, t, o));
}

//***************************************************************************
// Description: overlapping fields are rejected at init.
//***************************************************************************

static void test_bitmerge_rejects_overlap()
{
  CDescObject sd(g_i16_s);
  CIOVirt s(sd);
  CDescObject td(g_i16_t);
  CIOVirt t(td);
  CDescObject od(g_i16_o);
  CIOVirt o(od);

  TEST_ASSERT_EQUAL(-EINVAL, bitmergeInitScalar(g_overlap_bin, s, t, o));
}

//***************************************************************************
// Description: an input wider than 32 bits is rejected at init.
//***************************************************************************

static void test_bitmerge_rejects_unsupported_dtype()
{
  CDescObject sd(g_i16_s);
  CIOVirt s(sd);
  CDescObject td(g_u64_t);
  CIOVirt t(td);
  CDescObject od(g_i16_o);
  CIOVirt o(od);

  TEST_ASSERT_EQUAL(-EINVAL, bitmergeInitScalar(g_u64_bin, s, t, o));
}

//***************************************************************************
// Description: a batched input that cannot notify would never produce
// output, so it is rejected at init.
//***************************************************************************

static void test_bitmerge_rejects_batch_without_notify()
{
  CDescObject sd(g_i16_s);
  CIOVirt s(sd);
  CDescObject td(g_i16_t);
  CIOVirt t(td);
  CDescObject od(g_i16_o);
  CIOVirt o(od);
  CDescObject pd(g_fit_bin);
  CProgBitMerge p(pd);

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, o.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 4, false));
  TEST_ASSERT_EQUAL(OK, t.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BM_I16_S, &s);
  p.setObjectMapItem(BM_I16_T, &t);
  p.setObjectMapItem(BM_I16_O, &o);

  TEST_ASSERT_EQUAL(-EINVAL, p.init());
}

#ifdef CONFIG_DAWN_IO_TIMESTAMP
//***************************************************************************
// Description: a timestamped batch is padded, so the merge walks it batch
// by batch instead of as one flat array.
//***************************************************************************

static void test_bitmerge_tags_timestamped_batch()
{
  CDescObject sd(g_i16_ts);
  CIOVirt s(sd);
  CDescObject td(g_i16_t);
  CIOVirt t(td);
  CDescObject od(g_i16_o);
  CIOVirt o(od);
  CDescObject pd(g_ts_bin);
  CProgBitMerge p(pd);
  io_sdata_t<int16_t, 1, 4, true> sData;
  io_sdata_t<int16_t, 1, 1> tData;
  int16_t *out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, o.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 4, true));
  TEST_ASSERT_EQUAL(OK, t.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BM_I16_TS, &s);
  p.setObjectMapItem(BM_I16_T, &t);
  p.setObjectMapItem(BM_I16_O, &o);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  g_notified = nullptr;
  TEST_ASSERT_EQUAL(OK, o.setNotifier(bmNotifyCb, 0, nullptr));

  tData(0) = 3;
  TEST_ASSERT_EQUAL(OK, t.setData(tData));

  sData(0, 0) = 1;
  sData(0, 1) = 2;
  sData(0, 2) = 3;
  sData(0, 3) = 4;
  TEST_ASSERT_EQUAL(OK, s.setData(sData));

  TEST_ASSERT_NOT_NULL(g_notified);
  TEST_ASSERT_EQUAL(4, g_notified->getBatch());

  out = static_cast<int16_t *>(g_notified->getDataPtr(0));
  TEST_ASSERT_EQUAL(0x3001, out[0]);
  out = static_cast<int16_t *>(g_notified->getDataPtr(3));
  TEST_ASSERT_EQUAL(0x3004, out[0]);

  o.setNotifier(nullptr, 0, nullptr);
  TEST_ASSERT_EQUAL(OK, p.stop());
}
#endif

extern "C"
{
  int test_prog_bitmerge()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_bitmerge_tags_batch);
    DAWN_RUN_TEST(test_bitmerge_rejects_field_past_output_width);
    DAWN_RUN_TEST(test_bitmerge_merges_scalar_inputs);
    DAWN_RUN_TEST(test_bitmerge_slow_input_uses_element_zero);
    DAWN_RUN_TEST(test_bitmerge_rejects_empty_mask);
    DAWN_RUN_TEST(test_bitmerge_rejects_overlap);
    DAWN_RUN_TEST(test_bitmerge_rejects_unsupported_dtype);
    DAWN_RUN_TEST(test_bitmerge_rejects_batch_without_notify);
#ifdef CONFIG_DAWN_IO_TIMESTAMP
    DAWN_RUN_TEST(test_bitmerge_tags_timestamped_batch);
#endif
    return UNITY_END();
  }
}
