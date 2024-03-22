// dawn/tests/prog/test_toggle.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/toggle.hxx"
#include "test_common.hxx"

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

static constexpr auto TOGGLE_SRC = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 70);
static constexpr auto TOGGLE_DST = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 71);

static uint32_t g_cfg_src[] = {TOGGLE_SRC, 0};
static uint32_t g_cfg_dst[] = {TOGGLE_DST, 0};

static uint32_t g_bin_toggle[] = {
  CProgToggle::objectId(0),
  2,
  CProgToggle::cfgIdIOBind(2),
  TOGGLE_SRC,
  TOGGLE_DST,
  CProgToggle::cfgIdValues(),
  0, // off value
  1, // on value
};

static void test_toggle_rising_edge()
{
  CDescObject srcDesc(g_cfg_src);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_dst);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_toggle);
  CProgToggle prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(TOGGLE_SRC, &src);
  prog.setObjectMapItem(TOGGLE_DST, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  // First rising edge: 0→1, output should go from off(0) to on(1)
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  // Falling edge: 1→0, no toggle
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  // Second rising edge: 0→1, toggle back to off(0)
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  // Third rising edge: back to on
  in(0) = 1;                    // note: still 1 from previous, need 0→1 edge
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0)); // no edge, stays at 0

  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

static void test_toggle_reset()
{
  CDescObject srcDesc(g_cfg_src);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_dst);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_toggle);
  CProgToggle prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(TOGGLE_SRC, &src);
  prog.setObjectMapItem(TOGGLE_DST, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  // Toggle on
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  // Reset back to off
  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_RESET));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  // After reset, toggle state is off. But getData returns whatever is
  // in the CIOVirt (the last written value).  Reset just resets the
  // internal toggle state, it doesn't re-write the virtIO.
  // Next rising edge should go to on.
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_toggle()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_toggle_rising_edge);
    DAWN_RUN_TEST(test_toggle_reset);
    return UNITY_END();
  }
}
