// dawn/tests/io/test_sysinfo.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <fixedmath.h>

#include "dawn/io/sdata.hxx"
#include "dawn/io/sysinfo.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: cpuload IO (float)
//***************************************************************************

static uint32_t g_cfg_sysinfo_cpuload_float[] = {
  CIOSysinfo::objectIdCpuload(SObjectId::DTYPE_FLOAT),
  0,
};

//***************************************************************************
// Description: cpuload IO (ub16)
//***************************************************************************

static uint32_t g_cfg_sysinfo_cpuload_ub16[] = {
  CIOSysinfo::objectIdCpuload(SObjectId::DTYPE_UB16),
  0,
};

//***************************************************************************
// Description: uptime IO
//***************************************************************************

static uint32_t g_cfg_sysinfo_uptime[] = {
  CIOSysinfo::objectIdUptime(),
  0,
};

//***************************************************************************
// Description: test system IO uptime
//***************************************************************************

static void test_io_system_uptime()
{
  CDescObject desc(g_cfg_sysinfo_uptime);
  CIOSysinfo up(desc);
  io_sdata_t<uint64_t, 1> uptime1;
  io_sdata_t<uint64_t, 1> uptime2;
  io_sdata_t<uint64_t, 1> uptime3;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, up.configure());
  TEST_ASSERT_EQUAL(OK, up.init());

  // Get size

  TEST_ASSERT_EQUAL(up.getDataSize(), uptime1.getDataSize());
  TEST_ASSERT_EQUAL(up.getDataSize(), uptime2.getDataSize());
  TEST_ASSERT_EQUAL(up.getDataSize(), uptime3.getDataSize());

  // Not supported methods

  TEST_ASSERT_EQUAL(up.setData(uptime1), -ENOTSUP);
  TEST_ASSERT_EQUAL(up.getData(uptime1, 2), -ENOTSUP);

  // Get data

  sleep(1);
  TEST_ASSERT_EQUAL(up.getData(uptime1, 1), OK);
  sleep(1);
  TEST_ASSERT_EQUAL(up.getData(uptime2, 1), OK);
  sleep(1);
  TEST_ASSERT_EQUAL(up.getData(uptime3, 1), OK);

  TEST_ASSERT(uptime1(0) > 0);
  TEST_ASSERT(uptime2(0) > 0);
  TEST_ASSERT(uptime3(0) > 0);
  TEST_ASSERT(uptime3(0) >= uptime2(0));
  TEST_ASSERT(uptime2(0) >= uptime1(0));
}

//***************************************************************************
// Description: test system IO cpu load
//***************************************************************************

static void test_io_system_cpuload()
{
  CDescObject desc1(g_cfg_sysinfo_cpuload_float);
  CDescObject desc2(g_cfg_sysinfo_cpuload_ub16);
  CIOSysinfo cl1(desc1);
  CIOSysinfo cl2(desc1);
  io_sdata_t<float, 3> fcpuload;
  io_sdata_t<ub16_t, 3> ucpuload;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, cl1.configure());
  TEST_ASSERT_EQUAL(OK, cl1.init());
  TEST_ASSERT_EQUAL(OK, cl2.configure());
  TEST_ASSERT_EQUAL(OK, cl2.init());

  // Get size

  TEST_ASSERT_EQUAL(cl1.getDataSize(), fcpuload.getDataSize());
  TEST_ASSERT_EQUAL(cl2.getDataSize(), ucpuload.getDataSize());

  TEST_ASSERT_EQUAL(cl1.getData(fcpuload, 1), OK);
  TEST_ASSERT_EQUAL(cl2.getData(ucpuload, 1), OK);

  // CPU load not enabled in kernel now

  TEST_ASSERT_EQUAL(fcpuload(0), 0);
  TEST_ASSERT_EQUAL(fcpuload(1), 0);
  TEST_ASSERT_EQUAL(fcpuload(2), 0);

  TEST_ASSERT_EQUAL(ucpuload(0), 0);
  TEST_ASSERT_EQUAL(ucpuload(1), 0);
  TEST_ASSERT_EQUAL(ucpuload(2), 0);
}

extern "C"
{
  int test_io_system_sysinfo()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_system_uptime);
    DAWN_RUN_TEST(test_io_system_cpuload);

    return UNITY_END();
  }
}
