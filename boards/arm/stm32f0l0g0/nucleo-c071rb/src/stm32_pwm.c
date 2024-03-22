/*
 * boards/arm/stm32f0l0g0/nucleo-c071rb/src/stm32_pwm.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <errno.h>

#include <nuttx/timers/pwm.h>
#include <arch/board/board.h>

#include "stm32_pwm.h"

#include "nucleo-c071rb.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_pwm_setup
 *
 * Description:
 *   Initialize PWM and register the PWM device.
 *
 ****************************************************************************/

int stm32_pwm_setup(void)
{
  struct pwm_lowerhalf_s *pwm;
  int ret;

#ifdef CONFIG_STM32F0L0G0_TIM1_PWM
  pwm = stm32_pwminitialize(1);
  if (!pwm)
    {
      return -ENODEV;
    }

  ret = pwm_register("/dev/pwm0", pwm);
  if (ret < 0)
    {
      return ret;
    }
#endif

  return OK;
}
