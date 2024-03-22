/*
 * boards/arm/stm32f0l0g0/nucleo-c092rc/src/stm32_autoleds.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <stdint.h>
#include <stdbool.h>
#include <debug.h>

#include <nuttx/board.h>
#include <arch/board/board.h>

#include "chip.h"
#include "arm_internal.h"
#include "stm32_gpio.h"
#include "nucleo-c092rc.h"

#ifdef CONFIG_ARCH_LEDS

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_autoled_initialize
 ****************************************************************************/

void board_autoled_initialize(void)
{
  /* Configure LD1 and LD2 GPIO for output */

  stm32_configgpio(GPIO_LD1);
  stm32_configgpio(GPIO_LD2);
}

/****************************************************************************
 * Name: board_autoled_on
 ****************************************************************************/

void board_autoled_on(int led)
{
  if (led == BOARD_LED1)
    {
      stm32_gpiowrite(GPIO_LD1, true);
    }

  if (led == BOARD_LED2)
    {
      stm32_gpiowrite(GPIO_LD2, false);
    }
}

/****************************************************************************
 * Name: board_autoled_off
 ****************************************************************************/

void board_autoled_off(int led)
{
  if (led == BOARD_LED1)
    {
      stm32_gpiowrite(GPIO_LD1, false);
    }

  if (led == BOARD_LED2)
    {
      stm32_gpiowrite(GPIO_LD2, true);
    }
}

#endif /* CONFIG_ARCH_LEDS */
