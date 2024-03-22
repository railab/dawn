// dawn/tests/prog/test_iodemux.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/iodemux.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto DMX_CTRL = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 240);
static constexpr auto DMX_IN = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 241);
static constexpr auto DMX_OUT0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 242);
static constexpr auto DMX_OUT1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 243);

static uint32_t g_ctrl[] = {DMX_CTRL, 0};
static uint32_t g_in[] = {DMX_IN, 0};
static uint32_t g_out0[] = {DMX_OUT0, 0};
static uint32_t g_out1[] = {DMX_OUT1, 0};
static uint32_t g_prog[] = {
  CProgIODemux::objectId(0),
  3,
  CProgIODemux::cfgIdControl(),
  DMX_CTRL,
  CProgIODemux::cfgIdInput(),
  DMX_IN,
  CProgIODemux::cfgIdOutputs(2),
  DMX_OUT0,
  DMX_OUT1,
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
// Description: iodemux routes one input to the selected output only.
//***************************************************************************

static void test_iodemux_selects_output()
{
  CDescObject ctrld(g_ctrl);
  CIOVirt ctrl(ctrld);
  CDescObject ind(g_in);
  CIOVirt in(ind);
  CDescObject out0d(g_out0);
  CIOVirt out0(out0d);
  CDescObject out1d(g_out1);
  CIOVirt out1(out1d);
  CDescObject pd(g_prog);
  CProgIODemux prog(pd);

  initVirt(ctrl, true);
  initVirt(in, true);
  TEST_ASSERT_EQUAL(OK, out0.init());
  TEST_ASSERT_EQUAL(OK, out1.init());
  setU32(ctrl, 1);
  setU32(in, 55);

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(DMX_CTRL, &ctrl);
  prog.setObjectMapItem(DMX_IN, &in);
  prog.setObjectMapItem(DMX_OUT0, &out0);
  prog.setObjectMapItem(DMX_OUT1, &out1);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_EQUAL(55, getU32(out1));

  setU32(in, 66);
  TEST_ASSERT_EQUAL(66, getU32(out1));
  setU32(ctrl, 0);
  TEST_ASSERT_EQUAL(66, getU32(out0));
  setU32(in, 77);
  TEST_ASSERT_EQUAL(77, getU32(out0));
  TEST_ASSERT_EQUAL(66, getU32(out1));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_iodemux()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_iodemux_selects_output);

    return UNITY_END();
  }
}
