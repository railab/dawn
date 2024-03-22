// dawn/tests/prog/test_bitpack.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/bitpack.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto BP_BOOL_I0 = CIOVirt::objectId(SObjectId::DTYPE_BOOL, false, 120);
static constexpr auto BP_BOOL_I1 = CIOVirt::objectId(SObjectId::DTYPE_BOOL, false, 121);
static constexpr auto BP_U32_O = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 122);

static uint32_t g_bool_i0[] = {
  BP_BOOL_I0,
  0,
};

static uint32_t g_bool_i1[] = {
  BP_BOOL_I1,
  0,
};

static uint32_t g_u32_o[] = {
  BP_U32_O,
  0,
};

static uint32_t g_bool_bin[] = {
  CProgBitPack::objectId(0),
  2,
  CProgBitPack::cfgIdInputs(4),
  BP_BOOL_I0,
  0,
  BP_BOOL_I1,
  1,
  CProgBitPack::cfgIdOutput(),
  BP_U32_O,
};

static constexpr auto BP_U8_I = CIOVirt::objectId(SObjectId::DTYPE_UINT8, false, 123);
static constexpr auto BP_U64_O = CIOVirt::objectId(SObjectId::DTYPE_UINT64, false, 124);

static uint32_t g_u8_i[] = {
  BP_U8_I,
  0,
};

static uint32_t g_u64_o[] = {
  BP_U64_O,
  0,
};

static uint32_t g_u8_bin[] = {
  CProgBitPack::objectId(1),
  2,
  CProgBitPack::cfgIdInputs(2),
  BP_U8_I,
  0,
  CProgBitPack::cfgIdOutput(),
  BP_U64_O,
};

//***************************************************************************
// Description: bitpack combines BOOL inputs into a classic scalar bitmask.
//***************************************************************************

static void test_bitpack_combines_bool_inputs()
{
  CDescObject i0d(g_bool_i0);
  CIOVirt i0(i0d);
  CDescObject i1d(g_bool_i1);
  CIOVirt i1(i1d);
  CDescObject od(g_u32_o);
  CIOVirt o(od);
  CDescObject pd(g_bool_bin);
  CProgBitPack p(pd);
  io_sdata_t<uint8_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, i0.init());
  TEST_ASSERT_EQUAL(OK, i1.init());
  TEST_ASSERT_EQUAL(OK, o.init());
  TEST_ASSERT_EQUAL(OK, i0.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, i1.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BP_BOOL_I0, &i0);
  p.setObjectMapItem(BP_BOOL_I1, &i1);
  p.setObjectMapItem(BP_U32_O, &o);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  TEST_ASSERT_EQUAL(OK, i1.setData(in));
  TEST_ASSERT_EQUAL(OK, o.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  TEST_ASSERT_EQUAL(OK, o.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, i1.setData(in));
  TEST_ASSERT_EQUAL(OK, o.getData(out, 1));
  TEST_ASSERT_EQUAL(2, out(0));

  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  TEST_ASSERT_EQUAL(OK, o.getData(out, 1));
  TEST_ASSERT_EQUAL(3, out(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: bitpack flattens a dimensioned UINT8 input into a wider output
// slice starting at the configured bit offset.
//***************************************************************************

static void test_bitpack_packs_uint8_vector_to_uint64()
{
  CDescObject id(g_u8_i);
  CIOVirt in(id);
  CDescObject od(g_u64_o);
  CIOVirt outVio(od);
  CDescObject pd(g_u8_bin);
  CProgBitPack p(pd);
  io_sdata_t<uint8_t, 4, 1> inData;
  io_sdata_t<uint64_t, 1, 1> outData;

  TEST_ASSERT_EQUAL(OK, in.init());
  TEST_ASSERT_EQUAL(OK, outVio.init());
  TEST_ASSERT_EQUAL(OK, in.initialize(4, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BP_U8_I, &in);
  p.setObjectMapItem(BP_U64_O, &outVio);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  inData(0) = 0x11;
  inData(1) = 0x22;
  inData(2) = 0x33;
  inData(3) = 0x44;
  TEST_ASSERT_EQUAL(OK, in.setData(inData));
  TEST_ASSERT_EQUAL(OK, outVio.getData(outData, 1));
  TEST_ASSERT_EQUAL_UINT64(0x44332211ULL, outData(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

extern "C"
{
  int test_prog_bitpack()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_bitpack_combines_bool_inputs);
    DAWN_RUN_TEST(test_bitpack_packs_uint8_vector_to_uint64);
    return UNITY_END();
  }
}
