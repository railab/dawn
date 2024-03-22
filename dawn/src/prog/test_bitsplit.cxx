// dawn/tests/prog/test_bitsplit.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/bitsplit.hxx"
#include "test_common.hxx"

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

static constexpr auto BITSPLIT_SRC = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 60);
static constexpr auto BITSPLIT_D0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 61);
static constexpr auto BITSPLIT_D1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 62);

static uint32_t g_cfg_src[] = {BITSPLIT_SRC, 0};
static uint32_t g_cfg_d0[] = {BITSPLIT_D0, 0};
static uint32_t g_cfg_d1[] = {BITSPLIT_D1, 0};

static uint32_t g_bin_bitsplit[] = {
  CProgBitSplit::objectId(0),
  2,
  CProgBitSplit::cfgIdIOBind(4),
  BITSPLIT_SRC,
  BITSPLIT_D0,
  BITSPLIT_SRC,
  BITSPLIT_D1,
  CProgBitSplit::cfgIdBits(2),
  0, // bit 0 → d0
  1, // bit 1 → d1
};

static void test_bitsplit_extracts_bits()
{
  CDescObject srcDesc(g_cfg_src);
  CIOVirt src(srcDesc);
  CDescObject d0Desc(g_cfg_d0);
  CIOVirt d0(d0Desc);
  CDescObject d1Desc(g_cfg_d1);
  CIOVirt d1(d1Desc);
  CDescObject progDesc(g_bin_bitsplit);
  CProgBitSplit prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> o0;
  io_sdata_t<uint32_t, 1, 1> o1;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, d0.init());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(BITSPLIT_SRC, &src);
  prog.setObjectMapItem(BITSPLIT_D0, &d0);
  prog.setObjectMapItem(BITSPLIT_D1, &d1);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  // Input 0b11: bit0=1, bit1=1
  in(0) = 3;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, d0.getData(o0, 1));
  TEST_ASSERT_EQUAL(OK, d1.getData(o1, 1));
  TEST_ASSERT_EQUAL(1, o0(0));
  TEST_ASSERT_EQUAL(1, o1(0));

  // Input 0b10: bit0=0, bit1=1
  in(0) = 2;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, d0.getData(o0, 1));
  TEST_ASSERT_EQUAL(OK, d1.getData(o1, 1));
  TEST_ASSERT_EQUAL(0, o0(0));
  TEST_ASSERT_EQUAL(1, o1(0));

  // Input 0: both bits 0
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, d0.getData(o0, 1));
  TEST_ASSERT_EQUAL(OK, d1.getData(o1, 1));
  TEST_ASSERT_EQUAL(0, o0(0));
  TEST_ASSERT_EQUAL(0, o1(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_bitsplit()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_bitsplit_extracts_bits);
    return UNITY_END();
  }
}
