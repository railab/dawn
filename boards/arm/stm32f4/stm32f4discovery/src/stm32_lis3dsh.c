/*
 * boards/arm/stm32f4/stm32f4discovery/src/stm32_lis3dsh.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <debug.h>

#include <nuttx/spi/spi.h>
#include <nuttx/sensors/lis3dsh.h>

#include "stm32_spi.h"
#include "stm32f4discovery.h"

#ifdef CONFIG_SENSORS_LIS3DSH_UORB

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_lis3dsh_initialize
 *
 * Description:
 *   Initialise and register the on-board LIS3DSH accelerometer as a uORB
 *   sensor.
 *
 * Input Parameters:
 *   devno - The sensor device number (used to build the uORB topic path).
 *   busno - The SPI bus number (SPI1 = 1 on this board).
 *
 * Returned Value:
 *   Zero (OK) on success; a negated errno value on failure.
 *
 ****************************************************************************/

int board_lis3dsh_initialize(int devno, int busno)
{
  FAR struct spi_dev_s *spi;
  int ret;

  spi = stm32_spibus_initialize(busno);
  if (spi == NULL)
    {
      snerr("ERROR: Failed to initialize SPI bus %d\n", busno);
      return -ENODEV;
    }

  ret = lis3dsh_register_uorb(devno, spi);
  if (ret < 0)
    {
      snerr("ERROR: lis3dsh_register_uorb() failed: %d\n", ret);
    }

  return ret;
}

#endif /* CONFIG_SENSORS_LIS3DSH_UORB */
