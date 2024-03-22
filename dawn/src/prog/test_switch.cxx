// dawn/tests/prog/test_switch.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/switch.hxx"
#include "test_common.hxx"

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

static constexpr auto SW_IN0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 100);
static constexpr auto SW_IN1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 101);
static constexpr auto SW_TGT = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 102);

static uint32_t g_cfg_in0[] = {SW_IN0, 0};
static uint32_t g_cfg_in1[] = {SW_IN1, 0};
static uint32_t g_cfg_tgt[] = {SW_TGT, 0};

// Switch: both inputs must equal 1 to output onCmd=0x42 (arbitrary test
// value), otherwise 0x00.
static uint32_t g_bin_switch[] = {
  CProgSwitch::objectId(0),
  2,
  CProgSwitch::cfgIdInputs(4),
  SW_IN0,
  1,    // match value for in0
  SW_IN1,
  1,    // match value for in1
  CProgSwitch::cfgIdTarget(),
  SW_TGT,
  0x42, // onCmd
  0x00, // offCmd
};

static void test_switch_and_gate()
{
  CDescObject i0Desc(g_cfg_in0);
  CIOVirt in0(i0Desc);
  CDescObject i1Desc(g_cfg_in1);
  CIOVirt in1(i1Desc);
  CDescObject tDesc(g_cfg_tgt);
  CIOVirt tgt(tDesc);
  CDescObject progDesc(g_bin_switch);
  CProgSwitch prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, in0.init());
  TEST_ASSERT_EQUAL(OK, in1.init());
  TEST_ASSERT_EQUAL(OK, tgt.init());
  TEST_ASSERT_EQUAL(OK, in0.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, in1.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, tgt.initialize(1, 1, false));

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(SW_IN0, &in0);
  prog.setObjectMapItem(SW_IN1, &in1);
  prog.setObjectMapItem(SW_TGT, &tgt);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  // Neither matches yet → offCmd
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, in0.setData(in));
  TEST_ASSERT_EQUAL(OK, in1.setData(in));
  TEST_ASSERT_EQUAL(OK, tgt.getData(out, 1));
  TEST_ASSERT_EQUAL(0x00, out(0) & 0xff);

  // Partial match (only in0=1) → still off
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, in0.setData(in));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, in1.setData(in));
  TEST_ASSERT_EQUAL(OK, tgt.getData(out, 1));
  TEST_ASSERT_EQUAL(0x00, out(0) & 0xff);

  // Both match → onCmd
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, in0.setData(in));
  TEST_ASSERT_EQUAL(OK, in1.setData(in));
  TEST_ASSERT_EQUAL(OK, tgt.getData(out, 1));
  TEST_ASSERT_EQUAL(0x42, out(0) & 0xff);

  // Drop in0 → offCmd
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, in0.setData(in));
  TEST_ASSERT_EQUAL(OK, tgt.getData(out, 1));
  TEST_ASSERT_EQUAL(0x00, out(0) & 0xff);

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_switch()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_switch_and_gate);
    return UNITY_END();
  }
}
