// dawn/tests/prog/test_bitsplit.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/bitsplit.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto BS_SRC = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 60);
static constexpr auto BS_B0 = CIOVirt::objectId(SObjectId::DTYPE_BOOL, false, 61);
static constexpr auto BS_B1 = CIOVirt::objectId(SObjectId::DTYPE_BOOL, false, 62);
static constexpr auto BS_U16 = CIOVirt::objectId(SObjectId::DTYPE_UINT16, false, 63);

static uint32_t g_cfg_src[] = {
  BS_SRC,
  0,
};

static uint32_t g_cfg_b0[] = {
  BS_B0,
  0,
};

static uint32_t g_cfg_b1[] = {
  BS_B1,
  0,
};

static uint32_t g_bool_bin[] = {
  CProgBitSplit::objectId(0),
  2,
  CProgBitSplit::cfgIdIOBind(4),
  BS_SRC,
  BS_B0,
  BS_SRC,
  BS_B1,
  CProgBitSplit::cfgIdBits(2),
  0,
  1,
};

static uint32_t g_cfg_u16[] = {
  BS_U16,
  0,
};

static uint32_t g_u16_bin[] = {
  CProgBitSplit::objectId(1),
  2,
  CProgBitSplit::cfgIdIOBind(2),
  BS_SRC,
  BS_U16,
  CProgBitSplit::cfgIdBits(1),
  8,
};

//***************************************************************************
// Description: bitsplit extracts single BOOL bits when BOOL outputs are used.
//***************************************************************************

static void test_bitsplit_extracts_bool_bits()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject d0d(g_cfg_b0);
  CIOVirt d0(d0d);
  CDescObject d1d(g_cfg_b1);
  CIOVirt d1(d1d);
  CDescObject pd(g_bool_bin);
  CProgBitSplit p(pd);
  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint8_t, 1, 1> o0, o1;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d0.init());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BS_SRC, &s);
  p.setObjectMapItem(BS_B0, &d0);
  p.setObjectMapItem(BS_B1, &d1);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  in(0) = 3;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d0.getData(o0, 1));
  TEST_ASSERT_EQUAL(OK, d1.getData(o1, 1));
  TEST_ASSERT_EQUAL(1, o0(0));
  TEST_ASSERT_EQUAL(1, o1(0));

  in(0) = 2;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d0.getData(o0, 1));
  TEST_ASSERT_EQUAL(OK, d1.getData(o1, 1));
  TEST_ASSERT_EQUAL(0, o0(0));
  TEST_ASSERT_EQUAL(1, o1(0));

  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d0.getData(o0, 1));
  TEST_ASSERT_EQUAL(OK, d1.getData(o1, 1));
  TEST_ASSERT_EQUAL(0, o0(0));
  TEST_ASSERT_EQUAL(0, o1(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: bitsplit copies wider slices into non-BOOL outputs starting at
// the configured bit offset.
//***************************************************************************

static void test_bitsplit_extracts_uint16_slice()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject od(g_cfg_u16);
  CIOVirt out(od);
  CDescObject pd(g_u16_bin);
  CProgBitSplit p(pd);
  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint16_t, 1, 1> outData;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(BS_SRC, &s);
  p.setObjectMapItem(BS_U16, &out);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  in(0) = 0x44332211U;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  TEST_ASSERT_EQUAL_HEX16(0x3322, outData(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

extern "C"
{
  int test_prog_bitsplit()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_bitsplit_extracts_bool_bits);
    DAWN_RUN_TEST(test_bitsplit_extracts_uint16_slice);
    return UNITY_END();
  }
}
