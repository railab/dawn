// dawn/tests/prog/test_vecpack.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/vecpack.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto VP_I0 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 130);
static constexpr auto VP_I1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 131);
static constexpr auto VP_I2 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 132);
static constexpr auto VP_I3 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 133);
static constexpr auto VP_OUT = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 134);

static uint32_t g_i0[] = {
  VP_I0,
  0,
};

static uint32_t g_i1[] = {
  VP_I1,
  0,
};

static uint32_t g_i2[] = {
  VP_I2,
  0,
};

static uint32_t g_i3[] = {
  VP_I3,
  0,
};

static uint32_t g_out[] = {
  VP_OUT,
  0,
};

static uint32_t g_vecpack_bin[] = {
  CProgVecPack::objectId(0),
  2,
  CProgVecPack::cfgIdInputs(4),
  VP_I0,
  VP_I1,
  VP_I2,
  VP_I3,
  CProgVecPack::cfgIdOutput(),
  VP_OUT,
};

//***************************************************************************
// Description: vecpack combines scalar inputs into one cached vector output.
//***************************************************************************

static void test_vecpack_combines_scalars()
{
  CDescObject i0d(g_i0);
  CIOVirt i0(i0d);
  CDescObject i1d(g_i1);
  CIOVirt i1(i1d);
  CDescObject i2d(g_i2);
  CIOVirt i2(i2d);
  CDescObject i3d(g_i3);
  CIOVirt i3(i3d);
  CDescObject od(g_out);
  CIOVirt outVio(od);
  CDescObject pd(g_vecpack_bin);
  CProgVecPack p(pd);
  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 4, 1> out;

  TEST_ASSERT_EQUAL(OK, i0.init());
  TEST_ASSERT_EQUAL(OK, i1.init());
  TEST_ASSERT_EQUAL(OK, i2.init());
  TEST_ASSERT_EQUAL(OK, i3.init());
  TEST_ASSERT_EQUAL(OK, outVio.init());
  TEST_ASSERT_EQUAL(OK, p.configure());
  p.setObjectMapItem(VP_I0, &i0);
  p.setObjectMapItem(VP_I1, &i1);
  p.setObjectMapItem(VP_I2, &i2);
  p.setObjectMapItem(VP_I3, &i3);
  p.setObjectMapItem(VP_OUT, &outVio);
  TEST_ASSERT_EQUAL(OK, p.init());
  TEST_ASSERT_EQUAL(1, i0.getDataDim());
  TEST_ASSERT_FALSE(i0.isNotify());
  TEST_ASSERT_FALSE(i1.isNotify());
  TEST_ASSERT_FALSE(i2.isNotify());
  TEST_ASSERT_FALSE(i3.isNotify());
  TEST_ASSERT_EQUAL(4, outVio.getDataDim());
  TEST_ASSERT_EQUAL(OK, p.start());

  in(0) = 100;
  TEST_ASSERT_EQUAL(OK, i0.setData(in));
  in(0) = 300;
  TEST_ASSERT_EQUAL(OK, i2.setData(in));
  TEST_ASSERT_EQUAL(OK, outVio.getData(out, 1));
  TEST_ASSERT_EQUAL(100, out(0));
  TEST_ASSERT_EQUAL(0, out(1));
  TEST_ASSERT_EQUAL(300, out(2));
  TEST_ASSERT_EQUAL(0, out(3));

  TEST_ASSERT_EQUAL(OK, p.stop());
}

extern "C"
{
  int test_prog_vecpack()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_vecpack_combines_scalars);
    return UNITY_END();
  }
}
