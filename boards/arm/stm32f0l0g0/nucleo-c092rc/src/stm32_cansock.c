/*
 * boards/arm/stm32f0l0g0/nucleo-c092rc/src/stm32_cansock.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <debug.h>

#include "stm32_fdcan.h"
#include "nucleo-c092rc.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_cansock_setup
 *
 * Description:
 *  Initialize CAN socket interface
 *
 ****************************************************************************/

int stm32_cansock_setup(void)
{
  int ret;

  /* Configure STBY pin for output */

  stm32_configgpio(GPIO_FDCAN_STBY);

  /* Set STBY pin low */

  stm32_gpiowrite(GPIO_FDCAN_STBY, false);

  /* Call stm32_fdcaninitialize() to get an instance of the FDCAN interface */

  ret = stm32_fdcansockinitialize(1);
  if (ret < 0)
    {
      canerr("ERROR:  Failed to get FDCAN interface %d\n", ret);
      return ret;
    }

  return OK;
}
