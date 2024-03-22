// dawn/tests/prog/test_bitpack.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/bitpack.hxx"
#include "test_common.hxx"

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

static constexpr auto BP_IN0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 120);
static constexpr auto BP_IN1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 121);
static constexpr auto BP_OUT = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 122);

static uint32_t g_cfg_in0[] = {BP_IN0, 0};
static uint32_t g_cfg_in1[] = {BP_IN1, 0};
static uint32_t g_cfg_out[] = {BP_OUT, 0};

static uint32_t g_bin_bitpack[] = {
  CProgBitPack::objectId(0),
  2,
  CProgBitPack::cfgIdInputs(4),
  BP_IN0,
  0, // in0 → bit 0
  BP_IN1,
  1, // in1 → bit 1
  CProgBitPack::cfgIdOutput(),
  BP_OUT,
};

static void test_bitpack_combines_bits()
{
  CDescObject i0Desc(g_cfg_in0);
  CIOVirt in0(i0Desc);
  CDescObject i1Desc(g_cfg_in1);
  CIOVirt in1(i1Desc);
  CDescObject oDesc(g_cfg_out);
  CIOVirt out(oDesc);
  CDescObject progDesc(g_bin_bitpack);
  CProgBitPack prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> o;

  TEST_ASSERT_EQUAL(OK, in0.init());
  TEST_ASSERT_EQUAL(OK, in1.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, in0.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, in1.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, out.initialize(1, 1, false));

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(BP_IN0, &in0);
  prog.setObjectMapItem(BP_IN1, &in1);
  prog.setObjectMapItem(BP_OUT, &out);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  // Both 0
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, in0.setData(in));
  TEST_ASSERT_EQUAL(OK, in1.setData(in));
  TEST_ASSERT_EQUAL(OK, out.getData(o, 1));
  TEST_ASSERT_EQUAL(0, o(0));

  // bit0=1, bit1=0 → 0b01 = 1
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, in0.setData(in));
  TEST_ASSERT_EQUAL(OK, out.getData(o, 1));
  TEST_ASSERT_EQUAL(1, o(0));

  // bit0=0, bit1=1 → 0b10 = 2
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, in0.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, in1.setData(in));
  TEST_ASSERT_EQUAL(OK, out.getData(o, 1));
  TEST_ASSERT_EQUAL(2, o(0));

  // non-zero values treated as set
  in(0) = 42;
  TEST_ASSERT_EQUAL(OK, in0.setData(in));
  in(0) = 99;
  TEST_ASSERT_EQUAL(OK, in1.setData(in));
  TEST_ASSERT_EQUAL(OK, out.getData(o, 1));
  TEST_ASSERT_EQUAL(3, o(0)); // both bits set → 0b11

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_bitpack()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_bitpack_combines_bits);
    return UNITY_END();
  }
}
