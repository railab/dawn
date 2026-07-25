/****************************************************************************
 * boards/arm/nrf52/ppk2/include/board.h
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

#ifndef __BOARDS_ARM_NRF52_PPK2_INCLUDE_BOARD_H
#define __BOARDS_ARM_NRF52_PPK2_INCLUDE_BOARD_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <stdbool.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Clocking *****************************************************************/

/* The PPK2 (PCA63100) has a 32 MHz crystal for the HFCLK but no 32.768 kHz
 * crystal - P0.00 and P0.01 are used as GPIOs, so the LFCLK must be
 * generated from the internal RC oscillator or synthesized from the HFCLK.
 */

#define BOARD_SYSTICK_CLOCK         (64000000)

/* UART Pins ****************************************************************/

/* UART0 is available on test points only:
 *   UART0_RX - P1.10 - TP
 *   UART0_TX - P1.11 - TP
 */

#define BOARD_UART0_RX_PIN  (GPIO_INPUT  | GPIO_PORT1 | GPIO_PIN(10))
#define BOARD_UART0_TX_PIN  (GPIO_OUTPUT | GPIO_VALUE_ONE | GPIO_PORT1 | GPIO_PIN(11))

/* I2C Pins *****************************************************************/

/* I2C0 (TWI0) - 24CW160 EEPROM (0x50) and MCP4451 potentiometer (0x2c)
 *    I2C0_SCL - P0.25
 *    I2C0_SDA - P0.24
 */

#define BOARD_I2C0_SCL_PIN (GPIO_OUTPUT | GPIO_PORT0 | GPIO_PIN(25))
#define BOARD_I2C0_SDA_PIN (GPIO_INPUT  | GPIO_PORT0 | GPIO_PIN(24))

/* PWM Pins *****************************************************************/

/* PWM0 - RGB lightwell LEDs (FET low-side, active high):
 *   PWM0 CH0 - P1.05 - LEDR
 *   PWM0 CH1 - P1.06 - LEDG
 *   PWM0 CH2 - P1.07 - LEDB
 */

#define NRF52_PWM0_CH0_PIN (GPIO_OUTPUT | GPIO_PORT1 | GPIO_PIN(5))
#define NRF52_PWM0_CH1_PIN (GPIO_OUTPUT | GPIO_PORT1 | GPIO_PIN(6))
#define NRF52_PWM0_CH2_PIN (GPIO_OUTPUT | GPIO_PORT1 | GPIO_PIN(7))

/* ADC Pins *****************************************************************/

/* ADC
 *   ADC CH0 - P0.02 - AIN0 - VREF_IA (in-amp reference)
 *   ADC CH1 - P0.03 - AIN1 - VLDO sense
 *   ADC CH2 - P0.04 - AIN2 - VBB sense
 *   ADC CH3 - P0.05 - AIN3 - VIN sense
 *   ADC CH4 - P0.28 - AIN4 - VDUT sense
 *   ADC CH5 - P0.29 - AIN5 - AGND
 *   ADC CH6 - P0.30 - AIN6 - NTC temperature
 *   ADC CH7 - P0.31 - AIN7 - VSE_IA (in-amp output, current measurement)
 */

#define NRF52_ADC_CH0_PIN (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(2))
#define NRF52_ADC_CH1_PIN (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(3))
#define NRF52_ADC_CH2_PIN (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(4))
#define NRF52_ADC_CH3_PIN (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(5))
#define NRF52_ADC_CH4_PIN (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(28))
#define NRF52_ADC_CH5_PIN (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(29))
#define NRF52_ADC_CH6_PIN (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(30))
#define NRF52_ADC_CH7_PIN (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(31))

#endif /* __BOARDS_ARM_NRF52_PPK2_INCLUDE_BOARD_H */
