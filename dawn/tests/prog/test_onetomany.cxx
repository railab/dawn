// dawn/tests/prog/test_onetomany.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/onetomany.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto O2M_IN = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 233);
static constexpr auto O2M_OUT0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 234);
static constexpr auto O2M_OUT1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 235);

static uint32_t g_in[] = {O2M_IN, 0};
static uint32_t g_out0[] = {O2M_OUT0, 0};
static uint32_t g_out1[] = {O2M_OUT1, 0};
static uint32_t g_prog[] = {
  CProgOneToMany::objectId(0),
  2,
  CProgOneToMany::cfgIdInput(),
  O2M_IN,
  CProgOneToMany::cfgIdOutputs(2),
  O2M_OUT0,
  O2M_OUT1,
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
// Description: onetomany seeds all outputs and fans out source changes.
//***************************************************************************

static void test_onetomany_fans_out()
{
  CDescObject ind(g_in);
  CIOVirt in(ind);
  CDescObject out0d(g_out0);
  CIOVirt out0(out0d);
  CDescObject out1d(g_out1);
  CIOVirt out1(out1d);
  CDescObject pd(g_prog);
  CProgOneToMany prog(pd);

  initVirt(in, true);
  TEST_ASSERT_EQUAL(OK, out0.init());
  TEST_ASSERT_EQUAL(OK, out1.init());
  setU32(in, 12);

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(O2M_IN, &in);
  prog.setObjectMapItem(O2M_OUT0, &out0);
  prog.setObjectMapItem(O2M_OUT1, &out1);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_EQUAL(12, getU32(out0));
  TEST_ASSERT_EQUAL(12, getU32(out1));

  setU32(in, 34);
  TEST_ASSERT_EQUAL(34, getU32(out0));
  TEST_ASSERT_EQUAL(34, getU32(out1));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_onetomany()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_onetomany_fans_out);

    return UNITY_END();
  }
}
