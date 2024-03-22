// dawn/tests/prog/test_counter.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/counter.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto CT_SRC = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 80);
static constexpr auto CT_DST = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 81);

static uint32_t g_cfg_src[] = {
  CT_SRC,
  0,
};

static uint32_t g_cfg_dst[] = {
  CT_DST,
  0,
};

static uint32_t g_bin[] = {
  CProgCounter::objectId(0),
  2,
  CProgCounter::cfgIdIOBind(2),
  CT_SRC,
  CT_DST,
  CProgCounter::cfgIdParams(),
  0,
  3,
  1,
  0,
};

static uint32_t g_bin_initial_two[] = {
  CProgCounter::objectId(1),
  2,
  CProgCounter::cfgIdIOBind(2),
  CT_SRC,
  CT_DST,
  CProgCounter::cfgIdParams(),
  0,
  3,
  1,
  2,
};

//***************************************************************************
// Description: counter increments on rising edges and wraps at the limit.
//***************************************************************************

static void test_counter_increment()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject dd(g_cfg_dst);
  CIOVirt d(dd);
  CDescObject pd(g_bin);
  CProgCounter p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(CT_SRC, &s);
  p.setObjectMapItem(CT_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(2, out(0));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(3, out(0));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: counter ignores repeated high samples without a new edge.
//***************************************************************************

static void test_counter_no_edge_on_high()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject dd(g_cfg_dst);
  CIOVirt d(dd);
  CDescObject pd(g_bin);
  CProgCounter p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(CT_SRC, &s);
  p.setObjectMapItem(CT_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: reset clears the count before the next rising edge.
//***************************************************************************

static void test_counter_reset()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject dd(g_cfg_dst);
  CIOVirt d(dd);
  CDescObject pd(g_bin);
  CProgCounter p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(CT_SRC, &s);
  p.setObjectMapItem(CT_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
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
// Description: counter output starts at the configured initial value.
//***************************************************************************

static void test_counter_initial_value()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject dd(g_cfg_dst);
  CIOVirt d(dd);
  CDescObject pd(g_bin);
  CProgCounter p(pd);
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(CT_SRC, &s);
  p.setObjectMapItem(CT_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: counter uses the fourth parameter as initial/reset value.
//***************************************************************************

static void test_counter_nonzero_initial_and_reset()
{
  CDescObject sd(g_cfg_src);
  CIOVirt s(sd);
  CDescObject dd(g_cfg_dst);
  CIOVirt d(dd);
  CDescObject pd(g_bin_initial_two);
  CProgCounter p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, s.init());
  TEST_ASSERT_EQUAL(OK, d.init());
  TEST_ASSERT_EQUAL(OK, s.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(CT_SRC, &s);
  p.setObjectMapItem(CT_DST, &d);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(2, out(0));

  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, s.setData(in));
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(3, out(0));

  TEST_ASSERT_EQUAL(OK, p.trigger(CObject::CMD_RESET));
  TEST_ASSERT_EQUAL(OK, p.stop());
  TEST_ASSERT_EQUAL(OK, p.start());
  TEST_ASSERT_EQUAL(OK, d.getData(out, 1));
  TEST_ASSERT_EQUAL(2, out(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

extern "C"
{
  int test_prog_counter()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_counter_increment);
    DAWN_RUN_TEST(test_counter_no_edge_on_high);
    DAWN_RUN_TEST(test_counter_reset);
    DAWN_RUN_TEST(test_counter_initial_value);
    DAWN_RUN_TEST(test_counter_nonzero_initial_and_reset);
    return UNITY_END();
  }
}
