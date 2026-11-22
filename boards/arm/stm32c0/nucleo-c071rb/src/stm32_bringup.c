/*
 * boards/arm/stm32c0/nucleo-c071rb/src/stm32_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <debug.h>

#include <sys/types.h>

#ifdef CONFIG_INPUT_BUTTONS
#  include <nuttx/input/buttons.h>
#endif

#ifdef CONFIG_USERLED
#  include <nuttx/leds/userled.h>
#endif

#ifdef CONFIG_STM32_IWDG
#  include <stm32_wdg.h>
#endif

#ifdef CONFIG_LEDS_LED1642GW
#  include "stm32_led1642gw.h"
#endif

#include <arch/board/board.h>

#include "nucleo-c071rb.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 ****************************************************************************/

int stm32_bringup(void)
{
  int ret;

#ifdef CONFIG_STM32_IWDG
  /* Initialize the watchdog timer */

  stm32_iwdginitialize("/dev/watchdog0", STM32_LSI_FREQUENCY);
#endif

#ifdef CONFIG_USERLED
  /* Register the LED driver */

  ret = userled_lower_initialize("/dev/leds0");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: userled_lower_initialize() failed: %d\n", ret);
      return ret;
    }
#endif

#ifdef CONFIG_INPUT_BUTTONS
  /* Register the BUTTON driver */

  ret = btn_lower_initialize("/dev/buttons0");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: btn_lower_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_ADC
  /* Initialize ADC and register the ADC driver. */

  ret = stm32_adc_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_adc_setup failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_PWM
  /* Initialize PWM and register the PWM driver. */

  ret = stm32_pwm_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_pwm_setup failed: %d\n", ret);
    }
#endif

#if defined(CONFIG_LEDS_LED1642GW) && defined(CONFIG_RGBLED)
  /* Initialize the X-NUCLEO-LED16A1 and register its RGB LEDs. */

  ret = board_led1642gw_initialize(0, LED1642GW_NRGBLEDS, GPIO_LED1642GW_CLK,
                                   GPIO_LED1642GW_SDI, GPIO_LED1642GW_LE);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: board_led1642gw_initialize failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_PULSECOUNT
  /* Initialize pulsecount and register the pulsecount driver. */

  ret = stm32_pulsecount_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_pulsecount_setup failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_STM32_FDCAN_CHARDRIVER
  /* Initialize CAN and register the CAN driver. */

  ret = stm32_can_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_fdcan_setup failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_STM32_FDCAN_SOCKET
  /* Initialize CAN socket interface */

  ret = stm32_cansock_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: stm32_cansock_setup failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_FS_TMPFS
  /* Mount the tmpfs file system */

  ret = nx_mount(NULL, CONFIG_LIBC_TMPDIR, "tmpfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount tmpfs at %s: %d\n", CONFIG_LIBC_TMPDIR, ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_FILES
  /* Pre-populate fake files in tmpfs (must run after the mount above) */

  ret = dawn_fake_files_init();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: dawn_fake_files_init() failed: %d\n", ret);
    }
#endif

  UNUSED(ret);
  return OK;
}
