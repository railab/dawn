/*
 * boards/arm/stm32f4/stm32f4discovery/src/stm32_spi.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <stdbool.h>
#include <debug.h>

#include <nuttx/spi/spi.h>

#include <arch/board/board.h>

#include "arm_internal.h"
#include "chip.h"
#include "stm32.h"
#include "stm32f4discovery.h"

#ifdef CONFIG_STM32_SPI1

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_spidev_initialize
 *
 * Description:
 *   Called to configure SPI chip select GPIO pins for the STM32F4Discovery
 *   board.
 *
 ****************************************************************************/

void weak_function stm32_spidev_initialize(void)
{
#ifdef CONFIG_SENSORS_LIS3DSH_UORB
  /* On-board LIS3DSH MEMS accelerometer chip select */

  stm32_configgpio(GPIO_CS_MEMS);
#endif
}

/****************************************************************************
 * Name: stm32_spi1select and stm32_spi1status
 *
 * Description:
 *   The external functions, stm32_spi1select and stm32_spi1status must be
 *   provided by board-specific logic.  They are implementations of the
 *   select and status methods of the SPI interface defined by struct
 *   spi_ops_s (see include/nuttx/spi/spi.h).
 *
 ****************************************************************************/

void stm32_spi1select(struct spi_dev_s *dev, uint32_t devid, bool selected)
{
  spiinfo("devid: %d CS: %s\n", (int)devid, selected ? "assert" :
          "de-assert");

#ifdef CONFIG_SENSORS_LIS3DSH_UORB
  if (devid == SPIDEV_ACCELEROMETER(0))
    {
      /* Set the GPIO low to select and high to de-select */

      stm32_gpiowrite(GPIO_CS_MEMS, !selected);
    }
#endif
}

uint8_t stm32_spi1status(struct spi_dev_s *dev, uint32_t devid)
{
  return 0;
}

#endif /* CONFIG_STM32_SPI1 */
