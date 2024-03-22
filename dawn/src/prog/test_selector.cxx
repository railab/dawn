// dawn/tests/prog/test_selector.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/selector.hxx"
#include "test_common.hxx"

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

static constexpr auto SEL_CTRL = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 110);
static constexpr auto SEL_D0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 111);
static constexpr auto SEL_D1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 112);
static constexpr auto SEL_TGT = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 113);

static uint32_t g_cfg_ctrl[] = {SEL_CTRL, 0};
static uint32_t g_cfg_d0[] = {SEL_D0, 0};
static uint32_t g_cfg_d1[] = {SEL_D1, 0};
static uint32_t g_cfg_tgt[] = {SEL_TGT, 0};

static uint32_t g_bin_selector[] = {
  CProgSelector::objectId(0),
  3,
  CProgSelector::cfgIdControl(),
  SEL_CTRL,
  CProgSelector::cfgIdData(2),
  SEL_D0,
  SEL_D1,
  CProgSelector::cfgIdTarget(),
  SEL_TGT,
};

static void test_selector_routes_data()
{
  CDescObject ctrlDesc(g_cfg_ctrl);
  CIOVirt ctrl(ctrlDesc);
  CDescObject d0Desc(g_cfg_d0);
  CIOVirt d0(d0Desc);
  CDescObject d1Desc(g_cfg_d1);
  CIOVirt d1(d1Desc);
  CDescObject tgtDesc(g_cfg_tgt);
  CIOVirt tgt(tgtDesc);
  CDescObject progDesc(g_bin_selector);
  CProgSelector prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, ctrl.init());
  TEST_ASSERT_EQUAL(OK, d0.init());
  TEST_ASSERT_EQUAL(OK, d1.init());
  TEST_ASSERT_EQUAL(OK, tgt.init());

  TEST_ASSERT_EQUAL(OK, ctrl.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, d0.initialize(1, 1, false));
  TEST_ASSERT_EQUAL(OK, d1.initialize(1, 1, false));
  TEST_ASSERT_EQUAL(OK, tgt.initialize(1, 1, false));

  // Pre-load data into data virtIOs
  in(0) = 0xAAAA;
  TEST_ASSERT_EQUAL(OK, d0.setData(in));
  in(0) = 0xBBBB;
  TEST_ASSERT_EQUAL(OK, d1.setData(in));

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(SEL_CTRL, &ctrl);
  prog.setObjectMapItem(SEL_D0, &d0);
  prog.setObjectMapItem(SEL_D1, &d1);
  prog.setObjectMapItem(SEL_TGT, &tgt);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  // Set control to 0 — route d0 → tgt
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, ctrl.setData(in));
  TEST_ASSERT_EQUAL(OK, tgt.getData(out, 1));
  TEST_ASSERT_EQUAL(0xAAAA, out(0));

  // Set control to 1 — route d1 → tgt
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, ctrl.setData(in));
  TEST_ASSERT_EQUAL(OK, tgt.getData(out, 1));
  TEST_ASSERT_EQUAL(0xBBBB, out(0));

  // Out-of-range control → no write (tgt keeps previous value)
  in(0) = 5;
  TEST_ASSERT_EQUAL(OK, ctrl.setData(in));
  TEST_ASSERT_EQUAL(OK, tgt.getData(out, 1));
  TEST_ASSERT_EQUAL(0xBBBB, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_selector()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_selector_routes_data);
    return UNITY_END();
  }
}
