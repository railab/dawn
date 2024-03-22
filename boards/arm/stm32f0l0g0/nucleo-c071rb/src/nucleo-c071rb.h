/*
 * boards/arm/stm32f0l0g0/nucleo-c071rb/src/nucleo-c071rb.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#ifndef __BOARDS_ARM_STM32F0L0G0_NUCLEO_C071RB_SRC_NUCLEO_C071RB_H
#define __BOARDS_ARM_STM32F0L0G0_NUCLEO_C071RB_SRC_NUCLEO_C071RB_H

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

/* Nucleo-C071RB GPIOs ******************************************************/

/* User LEDs:
 *  LED1 - PA5
 *  LED2 - PC9
 */

#define GPIO_LD1        (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_MEDIUM | \
                         GPIO_OUTPUT_CLEAR | GPIO_PORTA | GPIO_PIN5)
#define GPIO_LD2        (GPIO_OUTPUT | GPIO_PUSHPULL | GPIO_SPEED_MEDIUM | \
                         GPIO_OUTPUT_SET | GPIO_PORTC | GPIO_PIN9)

/* Button definitions *******************************************************/

/*   B1 USER: the user button is connected to the I/O PC13 (pin 2) of the
 *   STM32 microcontroller.
 */

#define MIN_IRQBUTTON   BUTTON_USER
#define MAX_IRQBUTTON   BUTTON_USER
#define NUM_IRQBUTTONS  1

#define GPIO_BTN_USER   (GPIO_INPUT | GPIO_PULLUP | GPIO_EXTI | \
                         GPIO_PORTC | GPIO_PIN13)

/****************************************************************************
 * Public Types
 ****************************************************************************/

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
 * Name: stm32_adc_setup
 *
 * Description:
 *   Initialize ADC and register the ADC driver.
 *
 ****************************************************************************/

#ifdef CONFIG_ADC
int stm32_adc_setup(void);
#endif

/****************************************************************************
 * Name: stm32_pwm_setup
 *
 * Description:
 *   Initialize PWM and register the PWM driver.
 *
 ****************************************************************************/

#ifdef CONFIG_PWM
int stm32_pwm_setup(void);
#endif

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ARM_STM32F0L0G0_NUCLEO_C071RB_SRC_NUCLEO_C071RB_H */
