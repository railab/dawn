// dawn/tests/prog/test_manytoone.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/manytoone.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto M2O_IN0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 230);
static constexpr auto M2O_IN1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 231);
static constexpr auto M2O_OUT = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 232);

static uint32_t g_in0[] = {M2O_IN0, 0};
static uint32_t g_in1[] = {M2O_IN1, 0};
static uint32_t g_out[] = {M2O_OUT, 0};
static uint32_t g_prog[] = {
  CProgManyToOne::objectId(0),
  2,
  CProgManyToOne::cfgIdInputs(2),
  M2O_IN0,
  M2O_IN1,
  CProgManyToOne::cfgIdOutput(),
  M2O_OUT,
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
// Description: manytoone seeds from the first input, then the last changed
// input wins.
//***************************************************************************

static void test_manytoone_last_changed_wins()
{
  CDescObject in0d(g_in0);
  CIOVirt in0(in0d);
  CDescObject in1d(g_in1);
  CIOVirt in1(in1d);
  CDescObject outd(g_out);
  CIOVirt out(outd);
  CDescObject pd(g_prog);
  CProgManyToOne prog(pd);

  initVirt(in0, true);
  initVirt(in1, true);
  TEST_ASSERT_EQUAL(OK, out.init());
  setU32(in0, 11);
  setU32(in1, 22);

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(M2O_IN0, &in0);
  prog.setObjectMapItem(M2O_IN1, &in1);
  prog.setObjectMapItem(M2O_OUT, &out);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_EQUAL(11, getU32(out));

  setU32(in1, 33);
  TEST_ASSERT_EQUAL(33, getU32(out));
  setU32(in0, 44);
  TEST_ASSERT_EQUAL(44, getU32(out));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_manytoone()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_manytoone_last_changed_wins);

    return UNITY_END();
  }
}
