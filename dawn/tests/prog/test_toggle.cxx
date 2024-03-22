// dawn/tests/prog/test_toggle.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/toggle.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto TG_SRC = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 70);
static constexpr auto TG_DST = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 71);

static uint32_t g_cfg_src[] = {
  TG_SRC,
  0,
};

static uint32_t g_cfg_dst[] = {
  TG_DST,
  0,
};

static uint32_t g_bin[] = {
  CProgToggle::objectId(0),
  2,
  CProgToggle::cfgIdIOBind(2),
  TG_SRC,
  TG_DST,
  CProgToggle::cfgIdValues(),
  0,
  1,
};

//***************************************************************************
// Description: toggle output flips on each rising edge.
//***************************************************************************

static void test_toggle_rising_edge()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject dd(g_cfg_dst);
  CIOVirt d(dd);
  CDescObject pd(g_bin);
  CProgToggle p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(TG_SRC, &s);
  p.setObjectMapItem(TG_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: reset returns toggle output to its initial state.
//***************************************************************************

static void test_toggle_reset()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject dd(g_cfg_dst);
  CIOVirt d(dd);
  CDescObject pd(g_bin);
  CProgToggle p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(TG_SRC, &s);
  p.setObjectMapItem(TG_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));
  TEST_ASSERT_EQUAL(OK, p.trigger(CObject::CMD_RESET));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: toggle output starts at the configured initial value.
//***************************************************************************

static void test_toggle_initial_value()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject dd(g_cfg_dst);
  CIOVirt d(dd);
  CDescObject pd(g_bin);
  CProgToggle p(pd);
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(TG_SRC, &s);
  p.setObjectMapItem(TG_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

extern "C"
{
  int test_prog_toggle()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_toggle_rising_edge);
    DAWN_RUN_TEST(test_toggle_reset);
    DAWN_RUN_TEST(test_toggle_initial_value);
    return UNITY_END();
  }
}
