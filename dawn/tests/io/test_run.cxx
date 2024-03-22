// dawn/tests/io/test_run.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <debug.h>

#include <cstdio>

#include "test_common.hxx"

extern "C"
{
  // Common

  int test_io_data();
  int test_io_common();
  int test_io_handler();
  int test_io_factory();
  int test_io_notifier();
  int test_io_limits();

  // IO classes

  int test_io_config();
  int test_io_control();
  int test_io_trigger();
  int test_io_dummy();
  int test_io_dummy_notify();
  int test_io_virt();
  int test_io_sensor();
  int test_io_system_sysinfo();
  int test_io_system_boardctl();
  int test_io_system_uname();
  int test_io_system_uuid();
  int test_io_system_systime();
  int test_io_adc();
  int test_io_dac();
  int test_io_encoder();
  int test_io_encoder_index();
  int test_io_gpo();
  int test_io_gpi();
  int test_io_pwm();
  int test_io_buttons();
  int test_io_leds();
  int test_io_timerfd();
  int test_io_timestamp();
  int test_io_rand();
  int test_io_descriptor();
  int test_io_capabilities();
  int test_io_descselector();
  int test_io_fileio();
}

static int (*test_array[])(void) = {
  // Common

  test_io_data,
  test_io_common,
  test_io_handler,
  test_io_factory,
#ifdef CONFIG_DAWN_IO_LIMITS
  test_io_limits,
#endif
#ifdef CONFIG_DAWN_IO_NOTIFY
  test_io_notifier,
#endif

// IO classes

#ifdef CONFIG_DAWN_IO_CONFIG
  test_io_config,
#endif
#ifdef CONFIG_DAWN_IO_CONTROL
  test_io_control,
#endif
#ifdef CONFIG_DAWN_IO_TRIGGER
  test_io_trigger,
#endif
#ifdef CONFIG_DAWN_IO_DUMMY
  test_io_dummy,
#endif
#ifdef CONFIG_DAWN_IO_DUMMY_NOTIFY
  test_io_dummy_notify,
#endif
#ifdef CONFIG_DAWN_IO_VIRT
  test_io_virt,
#endif
#ifdef CONFIG_DAWN_IO_SENSOR
  test_io_sensor,
#endif
#ifdef CONFIG_DAWN_IO_SYSINFO
  test_io_system_sysinfo,
#endif
#ifdef CONFIG_DAWN_IO_BOARDCTL
  test_io_system_boardctl,
#endif
#ifdef CONFIG_DAWN_IO_UNAME
  test_io_system_uname,
#endif
#ifdef CONFIG_DAWN_IO_UUID
  test_io_system_uuid,
#endif
#ifdef CONFIG_DAWN_IO_SYSTIME
  test_io_system_systime,
#endif
#ifdef CONFIG_DAWN_IO_ADC
  test_io_adc,
#endif
#ifdef CONFIG_DAWN_IO_DAC
  test_io_dac,
#endif
#ifdef CONFIG_DAWN_IO_ENCODER
  test_io_encoder,
#endif
#ifdef CONFIG_DAWN_IO_ENCODER_INDEX
  test_io_encoder_index,
#endif
#ifdef CONFIG_DAWN_IO_GPI
  test_io_gpi,
#endif
#ifdef CONFIG_DAWN_IO_GPO
  test_io_gpo,
#endif
#ifdef CONFIG_DAWN_IO_BUTTONS
  test_io_buttons,
#endif
#ifdef CONFIG_DAWN_IO_LEDS
  test_io_leds,
#endif
#ifdef CONFIG_DAWN_IO_PWM
  test_io_pwm,
#endif
#ifdef CONFIG_DAWN_IO_TIMERFD
  test_io_timerfd,
#endif
#ifdef CONFIG_DAWN_IO_TIMESTAMPIO
  test_io_timestamp,
#endif
#ifdef CONFIG_DAWN_IO_RANDIO
  test_io_rand,
#endif
#ifdef CONFIG_DAWN_IO_DESCRIPTOR
  test_io_descriptor,
#endif
#ifdef CONFIG_DAWN_IO_CAPABILITIES
  test_io_capabilities,
#endif
#ifdef CONFIG_DAWN_IO_DESC_SELECTOR
  test_io_descselector,
#endif
#ifdef CONFIG_DAWN_IO_FILE
  test_io_fileio,
#endif

  nullptr,
};

extern "C"
{
  int test_run_io()
  {
    int ret = 0;
    int i = 0;
    bool fail = false;

    DAWN_TEST_SEPARATOR();

    for (i = 0; test_array[i] != nullptr; i += 1)
      {
        ret = test_array[i]();
        if (ret != 0)
          {
            fail = true;
          }
#ifdef CONFIG_DAWN_TEST_EXIT_ON_FAIL
        if (ret != 0)
          {
            printf("Force exit on the first fail!\n");
            goto errout;
          }
#endif
      }

#ifdef CONFIG_DAWN_TEST_EXIT_ON_FAIL
  errout:
#endif
    return fail ? -1 : 0;
  }
}
