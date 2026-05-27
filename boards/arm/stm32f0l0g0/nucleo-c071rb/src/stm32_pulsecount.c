/*
 * boards/arm/stm32f0l0g0/nucleo-c071rb/src/stm32_pulsecount.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <errno.h>

#include <nuttx/timers/pulsecount.h>

#include <arch/board/board.h>
#include <stm32_pulsecount.h>

#include "nucleo-c071rb.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_pulsecount_setup
 *
 * Description:
 *   Initialize pulsecount and register the pulsecount device.
 *
 ****************************************************************************/

int stm32_pulsecount_setup(void)
{
  struct pulsecount_lowerhalf_s *pulsecount;
  int ret;

#ifdef CONFIG_STM32F0L0G0_TIM1_PULSECOUNT
  pulsecount = stm32_pulsecountinitialize(1);
  if (pulsecount == NULL)
    {
      return -ENODEV;
    }

  ret = pulsecount_register("/dev/pulsecount0", pulsecount);
  if (ret < 0)
    {
      return ret;
    }
#endif

  return OK;
}
