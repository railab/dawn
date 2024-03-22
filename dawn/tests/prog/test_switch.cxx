// dawn/tests/prog/test_switch.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/switch.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto SW_I0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 100);
static constexpr auto SW_I1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 101);
static constexpr auto SW_T = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 102);
static constexpr auto SW_B0 = CIOVirt::objectId(SObjectId::DTYPE_BOOL, false, 103);

static uint32_t g_i0[] = {
  SW_I0,
  0,
};

static uint32_t g_i1[] = {
  SW_I1,
  0,
};

static uint32_t g_t[] = {
  SW_T,
  0,
};

static uint32_t g_b0[] = {
  SW_B0,
  0,
};

static uint32_t g_bin[] = {
  CProgSwitch::objectId(0),
  2,
  CProgSwitch::cfgIdInputs(4),
  SW_I0,
  1,
  SW_I1,
  1,
  CProgSwitch::cfgIdTarget(),
  SW_T,
  0x42,
  0x00,
};

static uint32_t g_bad_bin[] = {
  CProgSwitch::objectId(1),
  2,
  CProgSwitch::cfgIdInputs(2),
  SW_B0,
  1,
  CProgSwitch::cfgIdTarget(),
  SW_T,
  0x42,
  0x00,
};

//***************************************************************************
// Description: switch behaves as an AND gate and writes the configured
// on/off command to the target as input match conditions change.
//***************************************************************************

static void test_switch_and_gate()
{
  CDescObject i0d(g_i0);
  CIOVirt i0(i0d);
  CDescObject i1d(g_i1);
  CIOVirt i1(i1d);
  CDescObject td(g_t);
  CIOVirt t(td);
  CDescObject pd(g_bin);
  CProgSwitch p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, i0.init());
  TEST_ASSERT_EQUAL(OK, i1.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, i0.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, i1.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SW_I0, &i0);
  p.setObjectMapItem(SW_I1, &i1);
  p.setObjectMapItem(SW_T, &t);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  TEST_ASSERT_EQUAL(OK, i1.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0x00, out(0) & 0xff);
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, i1.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0x00, out(0) & 0xff);
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  TEST_ASSERT_EQUAL(OK, i1.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0x42, out(0) & 0xff);
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0x00, out(0) & 0xff);
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: switch writes the ON command during start() when all inputs are
// already in the matching state.
//***************************************************************************

static void test_switch_applies_seeded_match_on_start()
{
  CDescObject i0d(g_i0);
  CIOVirt i0(i0d);
  CDescObject i1d(g_i1);
  CIOVirt i1(i1d);
  CDescObject td(g_t);
  CIOVirt t(td);
  CDescObject pd(g_bin);
  CProgSwitch p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, i0.init());
  TEST_ASSERT_EQUAL(OK, i1.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, i0.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, i1.initialize(1, 1, true));

  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  TEST_ASSERT_EQUAL(OK, i1.setData(in));

  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SW_I0, &i0);
  p.setObjectMapItem(SW_I1, &i1);
  p.setObjectMapItem(SW_T, &t);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0x42, out(0) & 0xff);
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: switch rejects non-uint32 or non-scalar control inputs.
//***************************************************************************

static void test_switch_rejects_non_uint32_scalar_inputs()
{
  CDescObject bd(g_b0);
  CIOVirt b0(bd);
  CDescObject td(g_t);
  CIOVirt t(td);
  CDescObject pd(g_bad_bin);
  CProgSwitch p(pd);

  TEST_ASSERT_EQUAL(OK, b0.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, b0.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SW_B0, &b0);
  p.setObjectMapItem(SW_T, &t);
  TEST_ASSERT_EQUAL(-EINVAL, p.init());
}

extern "C"
{
  int test_prog_switch()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_switch_and_gate);
    DAWN_RUN_TEST(test_switch_applies_seeded_match_on_start);
    DAWN_RUN_TEST(test_switch_rejects_non_uint32_scalar_inputs);
    return UNITY_END();
  }
}
