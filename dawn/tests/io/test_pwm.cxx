// dawn/tests/io/test_pwm.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/pwm.hxx"
#include "dawn/io/config.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

#if CONFIG_PWM_NCHANNELS != 3
#  error this test require CONFIG_PWM_NCHANNELS == 3
#endif

static constexpr auto PWM0 = CIOPwm::objectId(false, 0);
static constexpr auto PWM_FREQ_CFG_IO = CIOConfig::objectId(SObjectId::DTYPE_UINT32, 140);

//***************************************************************************
// Description: test descriptors for PWM
//***************************************************************************

static uint32_t g_cfg_pwm0[] = {
  // Device: /dev/pwm0

  PWM0,
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

static uint32_t g_cfg_pwm0_freq[] = {
  // Device: /dev/pwm0

  PWM0,
  2,
  CIOCommon::cfgIdDevno(),
  0,
  CIOPwm::cfgIdFreq(true),
  1000,
};

static uint32_t g_cfg_pwm_freq_io[] = {
  PWM_FREQ_CFG_IO,
  2,
  CIOConfig::cfgIdCfg(),
  CIOPwm::cfgIdFreq(true),
  CIOConfig::cfgIdAlloc(SObjectId::DTYPE_UINT32, false, 1),
  PWM0,
};

//***************************************************************************
// Description: PWM read is not supported and returns -ENOTSUP.
//***************************************************************************

static void test_io_pwm_get_unsupported()
{
  CDescObject desc(g_cfg_pwm0);
  CIOPwm pwm(desc);
  io_sdata_t<uint32_t, 3> data;

  TEST_ASSERT_EQUAL(OK, pwm.configure());
  TEST_ASSERT_EQUAL(OK, pwm.init());

  TEST_ASSERT_EQUAL(-ENOTSUP, pwm.getData(data, 1));
}

//***************************************************************************
// Description: setData with the wrong dim count returns -EINVAL.
//***************************************************************************

static void test_io_pwm_set_wrong_dim_rejected()
{
  CDescObject desc(g_cfg_pwm0);
  CIOPwm pwm(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, pwm.configure());
  TEST_ASSERT_EQUAL(OK, pwm.init());

  data(0) = 10;
  TEST_ASSERT_EQUAL(-EINVAL, pwm.setData(data));
}

//***************************************************************************
// Description: setData with a per-channel value out of range returns
// -EINVAL.
//***************************************************************************

static void test_io_pwm_set_out_of_range_rejected()
{
  CDescObject desc(g_cfg_pwm0);
  CIOPwm pwm(desc);
  io_sdata_t<uint32_t, 3> data;

  TEST_ASSERT_EQUAL(OK, pwm.configure());
  TEST_ASSERT_EQUAL(OK, pwm.init());

  data(0) = 1900;
  data(1) = 1;
  data(2) = 1;
  TEST_ASSERT_EQUAL(-EINVAL, pwm.setData(data));
}

//***************************************************************************
// Description: setData with all-zero, max, and mid-range duty values is
// accepted.
//***************************************************************************

static void test_io_pwm_set_valid_values()
{
  CDescObject desc(g_cfg_pwm0);
  CIOPwm pwm(desc);
  io_sdata_t<uint32_t, 3> data;
  uint32_t values[] = {0, 1000, 500};
  size_t i;

  TEST_ASSERT_EQUAL(OK, pwm.configure());
  TEST_ASSERT_EQUAL(OK, pwm.init());

  for (i = 0; i < 3; i++)
    {
      data(0) = values[i];
      data(1) = values[i];
      data(2) = values[i];
      TEST_ASSERT_EQUAL(OK, pwm.setData(data));
    }
}

//***************************************************************************
// Description: PWM frequency is writable through runtime config.
//***************************************************************************

static void test_io_pwm_runtime_frequency_config()
{
  CDescObject desc(g_cfg_pwm0_freq);
  CIOPwm pwm(desc);
  uint32_t freq;

  TEST_ASSERT_EQUAL(OK, pwm.configure());
  TEST_ASSERT_EQUAL(OK, pwm.init());

  TEST_ASSERT_EQUAL(OK, pwm.getConfig(CIOPwm::cfgIdFreq(true), &freq, 1));
  TEST_ASSERT_EQUAL(1000, freq);

  freq = 0;
  TEST_ASSERT_EQUAL(-EINVAL, pwm.setConfig(CIOPwm::cfgIdFreq(true), &freq, 1));

  freq = 500;
  TEST_ASSERT_EQUAL(OK, pwm.setConfig(CIOPwm::cfgIdFreq(true), &freq, 1));
  freq = 0;
  TEST_ASSERT_EQUAL(OK, pwm.getConfig(CIOPwm::cfgIdFreq(true), &freq, 1));
  TEST_ASSERT_EQUAL(500, freq);
}

//***************************************************************************
// Description: PWM frequency is writable through ConfigIO.
//***************************************************************************

static void test_io_pwm_runtime_frequency_configio()
{
  CDescObject pwmDesc(g_cfg_pwm0_freq);
  CIOPwm pwm(pwmDesc);
  CDescObject cfgDesc(g_cfg_pwm_freq_io);
  CIOConfig cfg(cfgDesc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, pwm.configure());
  TEST_ASSERT_EQUAL(OK, pwm.init());
  TEST_ASSERT_EQUAL(OK, cfg.configure());
  TEST_ASSERT_EQUAL(OK, cfg.bind(&pwm, PWM0));

  data(0) = 750;
  TEST_ASSERT_EQUAL(OK, cfg.setData(data));

  data(0) = 0;
  TEST_ASSERT_EQUAL(OK, cfg.getData(data, 1));
  TEST_ASSERT_EQUAL(750, data(0));
}

extern "C"
{
  int test_io_pwm()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_pwm_get_unsupported);
    DAWN_RUN_TEST(test_io_pwm_set_wrong_dim_rejected);
    DAWN_RUN_TEST(test_io_pwm_set_out_of_range_rejected);
    DAWN_RUN_TEST(test_io_pwm_set_valid_values);
    DAWN_RUN_TEST(test_io_pwm_runtime_frequency_config);
    DAWN_RUN_TEST(test_io_pwm_runtime_frequency_configio);

    return UNITY_END();
  }
}
