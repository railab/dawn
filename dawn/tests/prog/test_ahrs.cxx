// dawn/tests/prog/test_ahrs.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cmath>

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/ahrs.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto FU_ACC = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 240);
static constexpr auto FU_GYR = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 241);
static constexpr auto FU_OUT = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 242);
static constexpr auto FU_MAG = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 243);

static constexpr float FU_G = 9.80665f;

static uint32_t g_acc[] = {FU_ACC, 0};
static uint32_t g_gyr[] = {FU_GYR, 0};
static uint32_t g_out[] = {FU_OUT, 0};
static uint32_t g_prog[] = {
  CProgAhrs::objectId(0),
  4,
  CProgAhrs::cfgIdAccel(),
  FU_ACC,
  CProgAhrs::cfgIdGyro(),
  FU_GYR,
  CProgAhrs::cfgIdOutput(),
  FU_OUT,
  CProgAhrs::cfgParams(true),
  SObjectCfg::fToCfg(0.5f),
  SObjectCfg::fToCfg(10.0f),
  SObjectCfg::fToCfg(5.0f),
  SObjectCfg::fToCfg(50.0f),
};

static uint32_t g_mag[] = {FU_MAG, 0};
static uint32_t g_prog_mag[] = {
  CProgAhrs::objectId(1),
  5,
  CProgAhrs::cfgIdAccel(),
  FU_ACC,
  CProgAhrs::cfgIdGyro(),
  FU_GYR,
  CProgAhrs::cfgIdMag(),
  FU_MAG,
  CProgAhrs::cfgIdOutput(),
  FU_OUT,
  CProgAhrs::cfgParams(true),
  SObjectCfg::fToCfg(0.5f),
  SObjectCfg::fToCfg(10.0f),
  SObjectCfg::fToCfg(5.0f),
  SObjectCfg::fToCfg(50.0f),
};

static int g_outNotify = 0;

static void initVirt(CIOVirt &io, bool notify)
{
  TEST_ASSERT_EQUAL(OK, io.init());
  TEST_ASSERT_EQUAL(OK, io.initialize(4, 1, notify));
}

static void setVec4(CIOCommon &io, float x, float y, float z, float w)
{
  io_sdata_t<float, 4, 1> data;

  data(0) = x;
  data(1) = y;
  data(2) = z;
  data(3) = w;
  TEST_ASSERT_EQUAL(OK, io.setData(data));
}

static void getVec3(CIOCommon &io, float *out)
{
  io_sdata_t<float, 3, 1> data;

  TEST_ASSERT_EQUAL(OK, io.getData(data, 1));
  out[0] = data(0);
  out[1] = data(1);
  out[2] = data(2);
}

static int outNotifierCb(void *priv, io_ddata_t *data)
{
  UNUSED(priv);
  UNUSED(data);
  g_outNotify++;
  return OK;
}

struct SAhrsRig
{
  CDescObject accd;
  CIOVirt acc;
  CDescObject gyrd;
  CIOVirt gyr;
  CDescObject outd;
  CIOVirt out;
  CDescObject pd;
  CProgAhrs prog;

  SAhrsRig()
    : accd(g_acc)
    , acc(accd)
    , gyrd(g_gyr)
    , gyr(gyrd)
    , outd(g_out)
    , out(outd)
    , pd(g_prog)
    , prog(pd)
  {
  }

  void up()
  {
    initVirt(acc, true);
    initVirt(gyr, true);
    TEST_ASSERT_EQUAL(OK, out.init());

    TEST_ASSERT_EQUAL(OK, prog.configure());
    prog.setObjectMapItem(FU_ACC, &acc);
    prog.setObjectMapItem(FU_GYR, &gyr);
    prog.setObjectMapItem(FU_OUT, &out);
    TEST_ASSERT_EQUAL(OK, prog.init());

    g_outNotify = 0;
    TEST_ASSERT_EQUAL(OK, out.setNotifier(outNotifierCb, 0, nullptr));
    TEST_ASSERT_EQUAL(OK, prog.start());
  }

  void down()
  {
    TEST_ASSERT_EQUAL(OK, prog.stop());
    out.setNotifier(nullptr, 0, nullptr);
  }
};

//***************************************************************************
// Description: the output virt is lazily initialized to dim 3 and a static
// device (any orientation) produces ~zero world-frame linear acceleration.
//***************************************************************************

static void test_ahrs_static_input_outputs_zero()
{
  SAhrsRig rig;

  rig.up();
  TEST_ASSERT_EQUAL(3, rig.out.getDataDim());

  // Static device, sensor Z up: accel measures +1 g on Z, gyro is silent.

  for (int i = 0; i < 100; i++)
    {
      setVec4(rig.acc, 0.0f, 0.0f, FU_G, 25.0f);
      setVec4(rig.gyr, 0.0f, 0.0f, 0.0f, 25.0f);
    }

  float lin[3];

  getVec3(rig.out, lin);
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 0.0f, lin[0]);
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 0.0f, lin[1]);
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 0.0f, lin[2]);

  rig.down();
}

//***************************************************************************
// Description: no output is produced until the first accel sample has been
// seen (gyro alone gives no attitude reference).
//***************************************************************************

static void test_ahrs_no_output_before_first_accel()
{
  SAhrsRig rig;

  rig.up();

  for (int i = 0; i < 10; i++)
    {
      setVec4(rig.gyr, 0.1f, 0.0f, 0.0f, 25.0f);
    }

  TEST_ASSERT_EQUAL(0, g_outNotify);

  setVec4(rig.acc, 0.0f, 0.0f, FU_G, 25.0f);
  TEST_ASSERT_EQUAL(0, g_outNotify);
  setVec4(rig.gyr, 0.0f, 0.0f, 0.0f, 25.0f);
  TEST_ASSERT_EQUAL(1, g_outNotify);

  rig.down();
}

//***************************************************************************
// Description: params are runtime-writable via setObjConfig; out-of-range
// values are rejected and keep the previous configuration.
//***************************************************************************

static void test_ahrs_params_runtime_update()
{
  SAhrsRig rig;

  rig.up();

  uint32_t good[4] = {
    SObjectCfg::fToCfg(1.5f),
    SObjectCfg::fToCfg(20.0f),
    SObjectCfg::fToCfg(5.0f),
    SObjectCfg::fToCfg(50.0f),
  };

  TEST_ASSERT_EQUAL(OK, rig.prog.setObjConfig(CProgAhrs::cfgParams(true), good, 4));

  uint32_t bad[4] = {
    SObjectCfg::fToCfg(100.0f),
    SObjectCfg::fToCfg(20.0f),
    SObjectCfg::fToCfg(5.0f),
    SObjectCfg::fToCfg(50.0f),
  };

  TEST_ASSERT_EQUAL(-EINVAL, rig.prog.setObjConfig(CProgAhrs::cfgParams(true), bad, 4));

  uint32_t back[4] = {0, 0, 0, 0};

  TEST_ASSERT_EQUAL(OK, rig.prog.getObjConfig(CProgAhrs::cfgParams(true), back, 4));
  TEST_ASSERT_EQUAL_FLOAT(1.5f, SObjectCfg::cfgToF(back[0]));
  TEST_ASSERT_EQUAL_FLOAT(20.0f, SObjectCfg::cfgToF(back[1]));

  rig.down();
}

//***************************************************************************
// Description: with the optional magnetometer bound, a static device (earth
// field on X) still produces ~zero world-frame linear acceleration.
//***************************************************************************

static void test_ahrs_static_with_magnetometer()
{
  CDescObject accd(g_acc);
  CIOVirt acc(accd);
  CDescObject gyrd(g_gyr);
  CIOVirt gyr(gyrd);
  CDescObject magd(g_mag);
  CIOVirt magio(magd);
  CDescObject outd(g_out);
  CIOVirt out(outd);
  CDescObject pd(g_prog_mag);
  CProgAhrs prog(pd);

  initVirt(acc, true);
  initVirt(gyr, true);
  initVirt(magio, true);
  TEST_ASSERT_EQUAL(OK, out.init());

  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(FU_ACC, &acc);
  prog.setObjectMapItem(FU_GYR, &gyr);
  prog.setObjectMapItem(FU_MAG, &magio);
  prog.setObjectMapItem(FU_OUT, &out);
  TEST_ASSERT_EQUAL(OK, prog.init());

  g_outNotify = 0;
  TEST_ASSERT_EQUAL(OK, out.setNotifier(outNotifierCb, 0, nullptr));
  TEST_ASSERT_EQUAL(OK, prog.start());

  for (int i = 0; i < 100; i++)
    {
      setVec4(acc, 0.0f, 0.0f, FU_G, 25.0f);
      setVec4(magio, 20.0f, 0.0f, -40.0f, 25.0f);
      setVec4(gyr, 0.0f, 0.0f, 0.0f, 25.0f);
    }

  TEST_ASSERT_EQUAL(100, g_outNotify);

  float lin[3];

  getVec3(out, lin);
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 0.0f, lin[0]);
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 0.0f, lin[1]);
  TEST_ASSERT_FLOAT_WITHIN(0.3f, 0.0f, lin[2]);

  TEST_ASSERT_EQUAL(OK, prog.stop());
  out.setNotifier(nullptr, 0, nullptr);
}

extern "C"
{
  int test_prog_ahrs()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_ahrs_static_input_outputs_zero);
    DAWN_RUN_TEST(test_ahrs_no_output_before_first_accel);
    DAWN_RUN_TEST(test_ahrs_params_runtime_update);
    DAWN_RUN_TEST(test_ahrs_static_with_magnetometer);

    return UNITY_END();
  }
}
