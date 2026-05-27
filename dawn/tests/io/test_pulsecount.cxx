// dawn/tests/io/test_pulsecount.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>
#include <fcntl.h>
#include <nuttx/timers/pulsecount.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/io/config.hxx"
#include "dawn/io/pulsecount.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto PULSECOUNT0 = CIOPulseCount::objectId(false, 0);
static constexpr auto PULSECOUNT_HIGH_CFG_IO = CIOConfig::objectId(SObjectId::DTYPE_UINT32, 141);

static uint32_t g_cfg_pulsecount0[] = {
  PULSECOUNT0,
  3,
  CIOCommon::cfgIdDevno(),
  0,
  CIOPulseCount::cfgIdHighNs(),
  1000,
  CIOPulseCount::cfgIdLowNs(),
  2000,
};

static int read_pulsecount_info(struct pulsecount_info_s *info)
{
  int fd;
  int ret;

  fd = open("/dev/pulsecount0", O_WRONLY | O_NONBLOCK);
  if (fd < 0)
    {
      return -errno;
    }

  ret = ioctl(fd, PULSECOUNTIOC_GETCHARACTERISTICS, reinterpret_cast<unsigned long>(info));
  if (ret < 0)
    {
      ret = -errno;
    }

  close(fd);
  return ret;
}

//***************************************************************************
// Description: pulsecount read is not supported and returns -ENOTSUP.
//***************************************************************************

static void test_io_pulsecount_get_unsupported()
{
  CDescObject desc(g_cfg_pulsecount0);
  CIOPulseCount pulsecount(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, pulsecount.configure());
  TEST_ASSERT_EQUAL(OK, pulsecount.init());

  TEST_ASSERT_EQUAL(-ENOTSUP, pulsecount.getData(data, 1));
}

//***************************************************************************
// Description: a zero pulse count is rejected.
//***************************************************************************

static void test_io_pulsecount_zero_rejected()
{
  CDescObject desc(g_cfg_pulsecount0);
  CIOPulseCount pulsecount(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, pulsecount.configure());
  TEST_ASSERT_EQUAL(OK, pulsecount.init());

  data(0) = 0;
  TEST_ASSERT_EQUAL(-EINVAL, pulsecount.setData(data));
}

//***************************************************************************
// Description: a valid write programs the NuttX pulsecount characteristics.
//***************************************************************************

static void test_io_pulsecount_set_valid_values()
{
  CDescObject desc(g_cfg_pulsecount0);
  CIOPulseCount pulsecount(desc);
  io_sdata_t<uint32_t, 1> data;
  struct pulsecount_info_s info = {0};

  TEST_ASSERT_EQUAL(OK, pulsecount.configure());
  TEST_ASSERT_EQUAL(OK, pulsecount.init());

  data(0) = 7;
  TEST_ASSERT_EQUAL(OK, pulsecount.setData(data));

  TEST_ASSERT_EQUAL(OK, read_pulsecount_info(&info));
  TEST_ASSERT_EQUAL(1000u, info.high_ns);
  TEST_ASSERT_EQUAL(2000u, info.low_ns);
  TEST_ASSERT_EQUAL(7u, info.count);
}

//***************************************************************************
// Description: high_ns and low_ns are writable through runtime config.
//***************************************************************************

static void test_io_pulsecount_runtime_config()
{
  uint32_t pulseCfg[] = {
    PULSECOUNT0,
    3,
    CIOCommon::cfgIdDevno(),
    0,
    CIOPulseCount::cfgIdHighNs(true),
    1000,
    CIOPulseCount::cfgIdLowNs(true),
    2000,
  };
  CDescObject desc(pulseCfg);
  CIOPulseCount pulsecount(desc);
  io_sdata_t<uint32_t, 1> data;
  struct pulsecount_info_s info = {0};
  uint32_t highNs;
  uint32_t lowNs;

  TEST_ASSERT_EQUAL(OK, pulsecount.configure());
  TEST_ASSERT_EQUAL(OK, pulsecount.init());

  TEST_ASSERT_EQUAL(OK, pulsecount.getConfig(CIOPulseCount::cfgIdHighNs(true), &highNs, 1));
  TEST_ASSERT_EQUAL(1000u, highNs);
  TEST_ASSERT_EQUAL(OK, pulsecount.getConfig(CIOPulseCount::cfgIdLowNs(true), &lowNs, 1));
  TEST_ASSERT_EQUAL(2000u, lowNs);

  highNs = 0;
  TEST_ASSERT_EQUAL(-EINVAL, pulsecount.setConfig(CIOPulseCount::cfgIdHighNs(true), &highNs, 1));

  highNs = 1500;
  lowNs = 2500;
  TEST_ASSERT_EQUAL(OK, pulsecount.setConfig(CIOPulseCount::cfgIdHighNs(true), &highNs, 1));
  TEST_ASSERT_EQUAL(OK, pulsecount.setConfig(CIOPulseCount::cfgIdLowNs(true), &lowNs, 1));

  data(0) = 5;
  TEST_ASSERT_EQUAL(OK, pulsecount.setData(data));

  TEST_ASSERT_EQUAL(OK, read_pulsecount_info(&info));
  TEST_ASSERT_EQUAL(1500u, info.high_ns);
  TEST_ASSERT_EQUAL(2500u, info.low_ns);
  TEST_ASSERT_EQUAL(5u, info.count);
}

//***************************************************************************
// Description: pulsecount timing is writable through ConfigIO.
//***************************************************************************

static void test_io_pulsecount_runtime_configio()
{
  uint32_t pulseCfg[] = {
    PULSECOUNT0,
    3,
    CIOCommon::cfgIdDevno(),
    0,
    CIOPulseCount::cfgIdHighNs(true),
    1000,
    CIOPulseCount::cfgIdLowNs(true),
    2000,
  };
  uint32_t cfgIo[] = {
    PULSECOUNT_HIGH_CFG_IO,
    2,
    CIOConfig::cfgIdCfg(),
    CIOPulseCount::cfgIdHighNs(true),
    CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 1),
    PULSECOUNT0,
  };
  CDescObject pulseDesc(pulseCfg);
  CIOPulseCount pulsecount(pulseDesc);
  CDescObject cfgDesc(cfgIo);
  CIOConfig cfg(cfgDesc);
  io_sdata_t<uint32_t, 1> data;
  struct pulsecount_info_s info = {0};

  TEST_ASSERT_EQUAL(OK, pulsecount.configure());
  TEST_ASSERT_EQUAL(OK, pulsecount.init());
  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&pulsecount, PULSECOUNT0));

  data(0) = 3333;
  TEST_ASSERT_EQUAL(OK, cfg.setData(data));

  data(0) = 9;
  TEST_ASSERT_EQUAL(OK, pulsecount.setData(data));

  TEST_ASSERT_EQUAL(OK, read_pulsecount_info(&info));
  TEST_ASSERT_EQUAL(3333u, info.high_ns);
  TEST_ASSERT_EQUAL(2000u, info.low_ns);
  TEST_ASSERT_EQUAL(9u, info.count);
}

extern "C"
{
  int test_io_pulsecount()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_pulsecount_get_unsupported);
    DAWN_RUN_TEST(test_io_pulsecount_zero_rejected);
    DAWN_RUN_TEST(test_io_pulsecount_set_valid_values);
    DAWN_RUN_TEST(test_io_pulsecount_runtime_config);
    DAWN_RUN_TEST(test_io_pulsecount_runtime_configio);

    return UNITY_END();
  }
}
