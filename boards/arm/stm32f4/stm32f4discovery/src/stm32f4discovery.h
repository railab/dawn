/*
 * boards/arm/stm32f4/stm32f4discovery/src/stm32f4discovery.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __BOARDS_ARM_STM32F4_STM32F4DISCOVERY_SRC_STM32F4DISCOVERY_H
#define __BOARDS_ARM_STM32F4_STM32F4DISCOVERY_SRC_STM32F4DISCOVERY_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>
#include <stdint.h>

#include "stm32_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* STM32F4 Discovery GPIOs **************************************************/

/* User LEDs:
 *  LED1 (green)  - PD12
 *  LED2 (orange) - PD13
 *  LED3 (red)    - PD14
 *  LED4 (blue)   - PD15
 */

#define GPIO_LED1       (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                         GPIO_OUTPUT_CLEAR|GPIO_PORTD|GPIO_PIN12)
#define GPIO_LED2       (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                         GPIO_OUTPUT_CLEAR|GPIO_PORTD|GPIO_PIN13)
#define GPIO_LED3       (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                         GPIO_OUTPUT_CLEAR|GPIO_PORTD|GPIO_PIN14)
#define GPIO_LED4       (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                         GPIO_OUTPUT_CLEAR|GPIO_PORTD|GPIO_PIN15)

/* Button definitions *******************************************************/

/*   B1 USER: the user button is connected to PA0 of the STM32
 *   microcontroller.  NOTE that an EXTI interrupt is configured.
 */

#define MIN_IRQBUTTON   BUTTON_USER
#define MAX_IRQBUTTON   BUTTON_USER
#define NUM_IRQBUTTONS  1

#define GPIO_BTN_USER   (GPIO_INPUT|GPIO_FLOAT|GPIO_EXTI|GPIO_PORTA|GPIO_PIN0)

/* SPI chip selects *********************************************************/

/* On-board LIS3DSH MEMS accelerometer chip select (PE3) */

#define GPIO_CS_MEMS    (GPIO_OUTPUT|GPIO_PUSHPULL|GPIO_SPEED_50MHz|\
                         GPIO_OUTPUT_SET|GPIO_PORTE|GPIO_PIN3)

/* USB OTG FS ***************************************************************/

/* PA9  OTG_FS_VBUS VBUS sensing (also connected to the green LED)
 * PC0  OTG_FS_PowerSwitchOn
 * PD5  OTG_FS_Overcurrent
 */

#define GPIO_OTGFS_VBUS   (GPIO_INPUT|GPIO_FLOAT|GPIO_SPEED_100MHz|\
                           GPIO_OPENDRAIN|GPIO_PORTA|GPIO_PIN9)
#define GPIO_OTGFS_PWRON  (GPIO_OUTPUT|GPIO_FLOAT|GPIO_SPEED_100MHz|\
                           GPIO_PUSHPULL|GPIO_PORTC|GPIO_PIN0)
#define GPIO_OTGFS_OVER   (GPIO_INPUT|GPIO_FLOAT|GPIO_SPEED_100MHz|\
                           GPIO_PUSHPULL|GPIO_PORTD|GPIO_PIN5)

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

/****************************************************************************
 * Public Functions Definitions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 ****************************************************************************/

int stm32_bringup(void);

/****************************************************************************
 * Name: stm32_usbinitialize
 *
 * Description:
 *   Called very early in initialization to setup USB-related GPIO pins for
 *   the STM32F4Discovery board.
 *
 ****************************************************************************/

#ifdef CONFIG_STM32_OTGFS
void weak_function stm32_usbinitialize(void);
#endif

/****************************************************************************
 * Name: stm32_spidev_initialize
 *
 * Description:
 *   Configure SPI chip-select GPIO pins for the STM32F4Discovery board.
 *
 ****************************************************************************/

#ifdef CONFIG_STM32_SPI1
void weak_function stm32_spidev_initialize(void);
#endif

/****************************************************************************
 * Name: board_lis3dsh_initialize
 *
 * Description:
 *   Initialise and register the on-board LIS3DSH accelerometer (uORB).
 *
 ****************************************************************************/

#ifdef CONFIG_SENSORS_LIS3DSH_UORB
int board_lis3dsh_initialize(int devno, int busno);
#endif

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ARM_STM32F4_STM32F4DISCOVERY_SRC_STM32F4DISCOVERY_H */
