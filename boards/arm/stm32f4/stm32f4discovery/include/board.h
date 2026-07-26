/*
 * boards/arm/stm32f4/stm32f4discovery/include/board.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __BOARDS_ARM_STM32F4_STM32F4DISCOVERY_INCLUDE_BOARD_H
#define __BOARDS_ARM_STM32F4_STM32F4DISCOVERY_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#ifndef __ASSEMBLY__
#  include <stdint.h>
#  include <stdbool.h>
#endif

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Clocking *****************************************************************/

/* The STM32F4 Discovery board features a single 8MHz crystal.  The canonical
 * configuration drives the system clock from the PLL (HSE) to 168 MHz and
 * derives a 48 MHz clock for USB OTG FS:
 *
 *   PLL_VCO = (HSE / PLLM) * PLLN = (8MHz / 8) * 336 = 336 MHz
 *   SYSCLK  = PLL_VCO / PLLP      = 336MHz / 2       = 168 MHz
 *   USB     = PLL_VCO / PLLQ      = 336MHz / 7       = 48 MHz
 */

/* HSI - 16 MHz RC factory-trimmed
 * LSI - 32 KHz RC
 * HSE - On-board crystal frequency is 8MHz
 * LSE - 32.768 kHz
 */

#define STM32_BOARD_XTAL        8000000ul

#define STM32_HSI_FREQUENCY     16000000ul
#define STM32_LSI_FREQUENCY     32000
#define STM32_HSE_FREQUENCY     STM32_BOARD_XTAL
#define STM32_LSE_FREQUENCY     32768

/* Main PLL Configuration (PLL source is HSE) */

#define STM32_PLLCFG_PLLM       RCC_PLLCFG_PLLM(8)
#define STM32_PLLCFG_PLLN       RCC_PLLCFG_PLLN(336)
#define STM32_PLLCFG_PLLP       RCC_PLLCFG_PLLP_2
#define STM32_PLLCFG_PLLQ       RCC_PLLCFG_PLLQ(7)

#define STM32_SYSCLK_FREQUENCY  168000000ul

/* AHB clock (HCLK) is SYSCLK (168MHz) */

#define STM32_RCC_CFGR_HPRE     RCC_CFGR_HPRE_SYSCLK  /* HCLK  = SYSCLK / 1 */
#define STM32_HCLK_FREQUENCY    STM32_SYSCLK_FREQUENCY

/* APB1 clock (PCLK1) is HCLK/4 (42MHz) */

#define STM32_RCC_CFGR_PPRE1    RCC_CFGR_PPRE1_HCLKd4  /* PCLK1 = HCLK / 4 */
#define STM32_PCLK1_FREQUENCY   (STM32_HCLK_FREQUENCY/4)

/* Timers driven from APB1 will be twice PCLK1 */

#define STM32_TIM2_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_TIM3_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_TIM4_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_TIM5_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_TIM6_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_TIM7_CLKIN   (2*STM32_PCLK1_FREQUENCY)
#define STM32_TIM12_CLKIN  (2*STM32_PCLK1_FREQUENCY)
#define STM32_TIM13_CLKIN  (2*STM32_PCLK1_FREQUENCY)
#define STM32_TIM14_CLKIN  (2*STM32_PCLK1_FREQUENCY)

/* APB2 clock (PCLK2) is HCLK/2 (84MHz) */

#define STM32_RCC_CFGR_PPRE2    RCC_CFGR_PPRE2_HCLKd2  /* PCLK2 = HCLK / 2 */
#define STM32_PCLK2_FREQUENCY   (STM32_HCLK_FREQUENCY/2)

/* Timers driven from APB2 will be twice PCLK2 */

#define STM32_TIM1_CLKIN   (2*STM32_PCLK2_FREQUENCY)
#define STM32_TIM8_CLKIN   (2*STM32_PCLK2_FREQUENCY)
#define STM32_TIM9_CLKIN   (2*STM32_PCLK2_FREQUENCY)
#define STM32_TIM10_CLKIN  (2*STM32_PCLK2_FREQUENCY)
#define STM32_TIM11_CLKIN  (2*STM32_PCLK2_FREQUENCY)

/* LED definitions **********************************************************/

/* LED index values for use with board_userled() */

#define BOARD_LED1        0
#define BOARD_LED2        1
#define BOARD_LED3        2
#define BOARD_LED4        3
#define BOARD_NLEDS       4

#define BOARD_LED_GREEN   BOARD_LED1
#define BOARD_LED_ORANGE  BOARD_LED2
#define BOARD_LED_RED     BOARD_LED3
#define BOARD_LED_BLUE    BOARD_LED4

/* LED bits for use with board_userled_all() */

#define BOARD_LED1_BIT    (1 << BOARD_LED1)
#define BOARD_LED2_BIT    (1 << BOARD_LED2)
#define BOARD_LED3_BIT    (1 << BOARD_LED3)
#define BOARD_LED4_BIT    (1 << BOARD_LED4)

/* If CONFIG_ARCH_LEDs is defined, then NuttX will control the 4 LEDs on
 * the board.  The following definitions describe how NuttX controls the LEDs:
 */

#define LED_STARTED       0  /* LED1 */
#define LED_HEAPALLOCATE  1  /* LED2 */
#define LED_IRQSENABLED   2  /* LED1 + LED2 */
#define LED_STACKCREATED  3  /* LED3 */
#define LED_INIRQ         4  /* LED1 + LED3 */
#define LED_SIGNAL        5  /* LED2 + LED3 */
#define LED_ASSERTION     6  /* LED1 + LED2 + LED3 */
#define LED_PANIC         7  /* N/C  + N/C  + N/C + LED4 */

/* Button definitions *******************************************************/

/* The STM32F4 Discovery supports one button: */

#define BUTTON_USER        0
#define NUM_BUTTONS        1
#define BUTTON_USER_BIT    (1 << BUTTON_USER)

/* Alternate function pin selections ****************************************/

/* USART2:
 *
 * The STM32F4 Discovery has no on-board serial devices; the console is
 * brought out to PA2 (TX) and PA3 (RX).  On this board they are bridged to
 * the ST-LINK Virtual COM Port.
 */

#define GPIO_USART2_RX  (GPIO_USART2_RX_1|GPIO_SPEED_100MHz)  /* PA3 */
#define GPIO_USART2_TX  (GPIO_USART2_TX_1|GPIO_SPEED_100MHz)  /* PA2 */

/* SPI - There is a MEMS device (LIS3DSH accelerometer) on SPI1 */

#define GPIO_SPI1_MISO  (GPIO_SPI1_MISO_1|GPIO_SPEED_50MHz)   /* PA6 */
#define GPIO_SPI1_MOSI  (GPIO_SPI1_MOSI_1|GPIO_SPEED_50MHz)   /* PA7 */
#define GPIO_SPI1_SCK   (GPIO_SPI1_SCK_1|GPIO_SPEED_50MHz)    /* PA5 */

/* USB OTG FS
 *
 * PA9  OTG_FS_VBUS VBUS sensing (also connected to the green LED)
 * PC0  OTG_FS_PowerSwitchOn
 * PD5  OTG_FS_Overcurrent
 */

#define GPIO_OTGFS_DM    (GPIO_OTGFS_DM_0|GPIO_SPEED_100MHz)
#define GPIO_OTGFS_DP    (GPIO_OTGFS_DP_0|GPIO_SPEED_100MHz)
#define GPIO_OTGFS_ID    (GPIO_OTGFS_ID_0|GPIO_SPEED_100MHz)
#define GPIO_OTGFS_SOF   (GPIO_OTGFS_SOF_0|GPIO_SPEED_100MHz)

#endif /* __BOARDS_ARM_STM32F4_STM32F4DISCOVERY_INCLUDE_BOARD_H */
