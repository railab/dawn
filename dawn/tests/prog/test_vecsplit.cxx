// dawn/tests/prog/test_vecsplit.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/vecsplit.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto VS_SRC = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 140);
static constexpr auto VS_O0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 141);
static constexpr auto VS_O1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 142);
static constexpr auto VS_O2 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 143);
static constexpr auto VS_O3 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 144);

static uint32_t g_src[] = {
  VS_SRC,
  0,
};

static uint32_t g_o0[] = {
  VS_O0,
  0,
};

static uint32_t g_o1[] = {
  VS_O1,
  0,
};

static uint32_t g_o2[] = {
  VS_O2,
  0,
};

static uint32_t g_o3[] = {
  VS_O3,
  0,
};

static uint32_t g_vecsplit_bin[] = {
  CProgVecSplit::objectId(0),
  2,
  CProgVecSplit::cfgIdSource(),
  VS_SRC,
  CProgVecSplit::cfgIdOutputs(4),
  VS_O0,
  VS_O1,
  VS_O2,
  VS_O3,
};

//***************************************************************************
// Description: vecsplit splits a vector input into scalar outputs.
//***************************************************************************

static void test_vecsplit_splits_vector()
{
  CDescObject sd(g_src);
  CIOVirt src(sd);
  CDescObject o0d(g_o0);
  CIOVirt o0(o0d);
  CDescObject o1d(g_o1);
  CIOVirt o1(o1d);
  CDescObject o2d(g_o2);
  CIOVirt o2(o2d);
  CDescObject o3d(g_o3);
  CIOVirt o3(o3d);
  CDescObject pd(g_vecsplit_bin);
  CProgVecSplit p(pd);
  io_sdata_t<uint32_t, 4, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, o0.init());
  TEST_ASSERT_EQUAL(OK, o1.init());
  TEST_ASSERT_EQUAL(OK, o2.init());
  TEST_ASSERT_EQUAL(OK, o3.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(4, 1, true));
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(VS_SRC, &src);
  p.setObjectMapItem(VS_O0, &o0);
  p.setObjectMapItem(VS_O1, &o1);
  p.setObjectMapItem(VS_O2, &o2);
  p.setObjectMapItem(VS_O3, &o3);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(1, o0.getDataDim());
  TEST_ASSERT_EQUAL(OK, p.start());

  in(0) = 11;
  in(1) = 22;
  in(2) = 33;
  in(3) = 44;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, o0.getData(out, 1));
  TEST_ASSERT_EQUAL(11, out(0));
  TEST_ASSERT_EQUAL(OK, o1.getData(out, 1));
  TEST_ASSERT_EQUAL(22, out(0));
  TEST_ASSERT_EQUAL(OK, o2.getData(out, 1));
  TEST_ASSERT_EQUAL(33, out(0));
  TEST_ASSERT_EQUAL(OK, o3.getData(out, 1));
  TEST_ASSERT_EQUAL(44, out(0));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

extern "C"
{
  int test_prog_vecsplit()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_vecsplit_splits_vector);
    return UNITY_END();
  }
}
