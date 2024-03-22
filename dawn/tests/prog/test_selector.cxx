// dawn/tests/prog/test_selector.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/selector.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto SL_C = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 110);
static constexpr auto SL_D0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 111);
static constexpr auto SL_D1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 112);
static constexpr auto SL_T = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 113);
static constexpr auto SL_D2 = CIOVirt::objectId(SObjectId::DTYPE_UINT8, false, 114);

static uint32_t g_c[] = {
  SL_C,
  0,
};

static uint32_t g_d0[] = {
  SL_D0,
  0,
};

static uint32_t g_d1[] = {
  SL_D1,
  0,
};

static uint32_t g_t[] = {
  SL_T,
  0,
};

static uint32_t g_d2[] = {
  SL_D2,
  0,
};

static uint32_t g_bin[] = {
  CProgSelector::objectId(0),
  3,
  CProgSelector::cfgIdControl(),
  SL_C,
  CProgSelector::cfgIdData(2),
  SL_D0,
  SL_D1,
  CProgSelector::cfgIdTarget(),
  SL_T,
};

static uint32_t g_bad_bin[] = {
  CProgSelector::objectId(1),
  3,
  CProgSelector::cfgIdControl(),
  SL_C,
  CProgSelector::cfgIdData(2),
  SL_D0,
  SL_D2,
  CProgSelector::cfgIdTarget(),
  SL_T,
};

//***************************************************************************
// Description: selector routes the currently selected data virtIO to the
// target and ignores out-of-range control values.
//***************************************************************************

static void test_selector_routes_data()
{
  CDescObject cd(g_c);
  CIOVirt c(cd);
  CDescObject d0d(g_d0);
  CIOVirt d0(d0d);
  CDescObject d1d(g_d1);
  CIOVirt d1(d1d);
  CDescObject td(g_t);
  CIOVirt t(td);
  CDescObject pd(g_bin);
  CProgSelector p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, c.init());
  TEST_ASSERT_EQUAL(OK, d0.init());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, c.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, d0.initialize(1, 1, false));
  TEST_ASSERT_EQUAL(OK, d1.initialize(1, 1, false));
  in(0) = 0xAAAA;
  TEST_ASSERT_EQUAL(OK, d0.setData(in));
  in(0) = 0xBBBB;
  TEST_ASSERT_EQUAL(OK, d1.setData(in));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SL_C, &c);
  p.setObjectMapItem(SL_D0, &d0);
  p.setObjectMapItem(SL_D1, &d1);
  p.setObjectMapItem(SL_T, &t);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, c.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0xAAAA, out(0));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, c.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0xBBBB, out(0));
  in(0) = 5;
  TEST_ASSERT_EQUAL(OK, c.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0xBBBB, out(0));
  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: selector applies the already seeded control value during
// start(), so the target is initialized with the selected data path
// immediately.
//***************************************************************************

static void test_selector_applies_seeded_control_on_start()
{
  CDescObject cd(g_c);
  CIOVirt c(cd);
  CDescObject d0d(g_d0);
  CIOVirt d0(d0d);
  CDescObject d1d(g_d1);
  CIOVirt d1(d1d);
  CDescObject td(g_t);
  CIOVirt t(td);
  CDescObject pd(g_bin);
  CProgSelector p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, c.init());
  TEST_ASSERT_EQUAL(OK, d0.init());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, c.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, d0.initialize(1, 1, false));
  TEST_ASSERT_EQUAL(OK, d1.initialize(1, 1, false));

  in(0) = 0xAAAA;
  TEST_ASSERT_EQUAL(OK, d0.setData(in));
  in(0) = 0xBBBB;
  TEST_ASSERT_EQUAL(OK, d1.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, c.setData(in));

  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SL_C, &c);
  p.setObjectMapItem(SL_D0, &d0);
  p.setObjectMapItem(SL_D1, &d1);
  p.setObjectMapItem(SL_T, &t);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0xBBBB, out(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: selector refreshes the control value before handling a data
// notification. This keeps stale cached indexes from routing inactive data.
//***************************************************************************

static void test_selector_data_notify_refreshes_control()
{
  CDescObject cd(g_c);
  CIOVirt c(cd);
  CDescObject d0d(g_d0);
  CIOVirt d0(d0d);
  CDescObject d1d(g_d1);
  CIOVirt d1(d1d);
  CDescObject td(g_t);
  CIOVirt t(td);
  CDescObject pd(g_bin);
  CProgSelector p(pd);
  io_sdata_t<uint32_t, 1, 1> in, out;

  TEST_ASSERT_EQUAL(OK, c.init());
  TEST_ASSERT_EQUAL(OK, d0.init());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, c.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, d0.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, d1.initialize(1, 1, true));

  in(0) = 0xAAAA;
  TEST_ASSERT_EQUAL(OK, d0.setData(in));
  in(0) = 0xBBBB;
  TEST_ASSERT_EQUAL(OK, d1.setData(in));

  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SL_C, &c);
  p.setObjectMapItem(SL_D0, &d0);
  p.setObjectMapItem(SL_D1, &d1);
  p.setObjectMapItem(SL_T, &t);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(OK, p.start());

  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, c.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0xBBBB, out(0));

  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, c.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0xAAAA, out(0));

  in(0) = 0xCCCC;
  TEST_ASSERT_EQUAL(OK, d1.setData(in));
  TEST_ASSERT_EQUAL(OK, t.getData(out, 1));
  TEST_ASSERT_EQUAL(0xAAAA, out(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

//***************************************************************************
// Description: selector rejects data inputs that do not share the same dtype
// and dimension contract.
//***************************************************************************

static void test_selector_rejects_mixed_data_shapes()
{
  CDescObject cd(g_c);
  CIOVirt c(cd);
  CDescObject d0d(g_d0);
  CIOVirt d0(d0d);
  CDescObject d2d(g_d2);
  CIOVirt d2(d2d);
  CDescObject td(g_t);
  CIOVirt t(td);
  CDescObject pd(g_bad_bin);
  CProgSelector p(pd);

  TEST_ASSERT_EQUAL(OK, c.init());
  TEST_ASSERT_EQUAL(OK, d0.init());
  TEST_ASSERT_EQUAL(OK, d2.init());
  TEST_ASSERT_EQUAL(OK, t.init());
  TEST_ASSERT_EQUAL(OK, c.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, d0.initialize(1, 1, false));
  TEST_ASSERT_EQUAL(OK, d2.initialize(1, 1, false));

  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(SL_C, &c);
  p.setObjectMapItem(SL_D0, &d0);
  p.setObjectMapItem(SL_D2, &d2);
  p.setObjectMapItem(SL_T, &t);
  TEST_ASSERT_EQUAL(-EINVAL, p.init());
}

extern "C"
{
  int test_prog_selector()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_selector_routes_data);
    DAWN_RUN_TEST(test_selector_applies_seeded_control_on_start);
    DAWN_RUN_TEST(test_selector_data_notify_refreshes_control);
    DAWN_RUN_TEST(test_selector_rejects_mixed_data_shapes);
    return UNITY_END();
  }
}
