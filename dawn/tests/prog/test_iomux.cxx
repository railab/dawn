// dawn/tests/prog/test_iomux.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/iomux.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto MUX_CTRL = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 236);
static constexpr auto MUX_IN0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 237);
static constexpr auto MUX_IN1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 238);
static constexpr auto MUX_OUT = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 239);

static uint32_t g_ctrl[] = {MUX_CTRL, 0};
static uint32_t g_in0[] = {MUX_IN0, 0};
static uint32_t g_in1[] = {MUX_IN1, 0};
static uint32_t g_out[] = {MUX_OUT, 0};
static uint32_t g_prog[] = {
  CProgIOMux::objectId(0),
  3,
  CProgIOMux::cfgIdControl(),
  MUX_CTRL,
  CProgIOMux::cfgIdInputs(2),
  MUX_IN0,
  MUX_IN1,
  CProgIOMux::cfgIdOutput(),
  MUX_OUT,
};

static void initVirt(CIOVirt &io, bool notify)
{
  TEST_ASSERT_EQUAL(OK, io.init());
  TEST_ASSERT_EQUAL(OK, io.initialize(1, 1, notify));
}

static void setU32(CIOCommon &io, uint32_t value)
{
  io_sdata_t<uint32_t, 1, 1> data;

  data(0) = value;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
}

static uint32_t getU32(CIOCommon &io)
{
  io_sdata_t<uint32_t, 1, 1> data;

  TEST_ASSERT_EQUAL(OK, io.getData(data, 1));
  return data(0);
}

//***************************************************************************
// Description: iomux routes only the selected input and ignores invalid
// selected indexes.
//***************************************************************************

static void test_iomux_selects_input()
{
  CDescObject ctrld(g_ctrl);
  CIOVirt ctrl(ctrld);
  CDescObject in0d(g_in0);
  CIOVirt in0(in0d);
  CDescObject in1d(g_in1);
  CIOVirt in1(in1d);
  CDescObject outd(g_out);
  CIOVirt out(outd);
  CDescObject pd(g_prog);
  CProgIOMux prog(pd);

  initVirt(ctrl, true);
  initVirt(in0, true);
  initVirt(in1, true);
  TEST_ASSERT_EQUAL(OK, out.init());
  setU32(ctrl, 1);
  setU32(in0, 10);
  setU32(in1, 20);

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(MUX_CTRL, &ctrl);
  prog.setObjectMapItem(MUX_IN0, &in0);
  prog.setObjectMapItem(MUX_IN1, &in1);
  prog.setObjectMapItem(MUX_OUT, &out);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_EQUAL(20, getU32(out));

  setU32(in0, 11);
  TEST_ASSERT_EQUAL(20, getU32(out));
  setU32(in1, 21);
  TEST_ASSERT_EQUAL(21, getU32(out));
  setU32(ctrl, 0);
  TEST_ASSERT_EQUAL(11, getU32(out));
  setU32(ctrl, 5);
  TEST_ASSERT_EQUAL(11, getU32(out));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_iomux()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_iomux_selects_input);

    return UNITY_END();
  }
}
