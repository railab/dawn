/*
 * boards/arm/stm32f0l0g0/nucleo-c092rc/src/stm32_can.c
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
 * Name: stm32_can_setup
 *
 * Description:
 *  Initialize CAN and register the CAN device
 *
 ****************************************************************************/

int stm32_can_setup(void)
{
  struct can_dev_s *can;
  int ret;

  /* Configure STBY pin for output */

  stm32_configgpio(GPIO_FDCAN_STBY);

  /* Set STBY pin low */

  stm32_gpiowrite(GPIO_FDCAN_STBY, false);

  /* Call stm32_fdcaninitialize() to get an instance of the CAN interface */

  can = stm32_fdcaninitialize(1);
  if (can == NULL)
    {
      canerr("ERROR:  Failed to get CAN interface\n");
      return -ENODEV;
    }

  /* Register the CAN driver at "/dev/can0" */

  ret = can_register("/dev/can0", can);
  if (ret < 0)
    {
      canerr("ERROR: can_register failed: %d\n", ret);
      return ret;
    }

  return OK;
}
