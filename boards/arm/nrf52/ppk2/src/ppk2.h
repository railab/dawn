/****************************************************************************
 * boards/arm/nrf52/ppk2/src/ppk2.h
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

#ifndef __BOARDS_ARM_NRF52_PPK2_SRC_PPK2_H
#define __BOARDS_ARM_NRF52_PPK2_SRC_PPK2_H

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>
#include <nuttx/compiler.h>

#include "nrf52_gpio.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Configuration ************************************************************/

/* procfs File System */

#ifdef CONFIG_FS_PROCFS
#  ifdef CONFIG_NSH_PROC_MOUNTPOINT
#    define NRF52_PROCFS_MOUNTPOINT CONFIG_NSH_PROC_MOUNTPOINT
#  else
#    define NRF52_PROCFS_MOUNTPOINT "/proc"
#  endif
#endif

/* I2C addresses ************************************************************/

#define PPK2_MCP4451_ADDR  (0x2c)  /* MCP4451 quad potentiometer */
#define PPK2_EEPROM_ADDR   (0x50)  /* 24CW160 calibration EEPROM */

/* Range switch status ******************************************************/

/* Measurement range switch state, level-shifted from the automatic
 * switching comparators.  The same nets drive the range-force FETs
 * (see the PCA63100 current measurement sheet).
 */

#define GPIO_SW1_ONOFF (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(0))
#define GPIO_SW2_ONOFF (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(1))
#define GPIO_SW3_ONOFF (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(26))
#define GPIO_SW4_ONOFF (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(27))

/* Power path control *******************************************************/

/* All enables default inactive - the board must power up with the DUT
 * output off.
 *
 * VEXT_EN, VLDO_EN and VOUT_EN drive high-side P-FET power path switches
 * and are ACTIVE LOW (verified on hardware: driving them high disconnects
 * the path), so their inactive boot state is high. The GPIO character
 * devices hide the polarity - a written 1 always means "switch closed"
 * (see g_gpiooutinvert in nrf52_gpio.c).
 */

#define GPIO_VEXT_EN (GPIO_OUTPUT | GPIO_VALUE_ONE | GPIO_PORT0 | GPIO_PIN(6))
#define GPIO_VLDO_EN (GPIO_OUTPUT | GPIO_VALUE_ONE | GPIO_PORT0 | GPIO_PIN(7))
#define GPIO_ANA_EN  (GPIO_OUTPUT | GPIO_VALUE_ZERO | GPIO_PORT0 | GPIO_PIN(8))
#define GPIO_REG_EN  (GPIO_OUTPUT | GPIO_VALUE_ZERO | GPIO_PORT1 | GPIO_PIN(13))
#define GPIO_VOUT_EN (GPIO_OUTPUT | GPIO_VALUE_ONE | GPIO_PORT1 | GPIO_PIN(4))

/* Calibration loads ********************************************************/

#define GPIO_CAL100K (GPIO_OUTPUT | GPIO_VALUE_ZERO | GPIO_PORT0 | GPIO_PIN(20))
#define GPIO_CAL10K  (GPIO_OUTPUT | GPIO_VALUE_ZERO | GPIO_PORT0 | GPIO_PIN(21))
#define GPIO_CAL1K   (GPIO_OUTPUT | GPIO_VALUE_ZERO | GPIO_PORT0 | GPIO_PIN(22))
#define GPIO_CAL100  (GPIO_OUTPUT | GPIO_VALUE_ZERO | GPIO_PORT0 | GPIO_PIN(23))

/* Logic port ***************************************************************/

#define GPIO_LP_EN (GPIO_OUTPUT | GPIO_VALUE_ZERO | GPIO_PORT0 | GPIO_PIN(9))
#define GPIO_LP_D0 (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(10))
#define GPIO_LP_D1 (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(11))
#define GPIO_LP_D2 (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(12))
#define GPIO_LP_D3 (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(13))
#define GPIO_LP_D4 (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(14))
#define GPIO_LP_D5 (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(15))
#define GPIO_LP_D6 (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(16))
#define GPIO_LP_D7 (GPIO_INPUT | GPIO_PORT0 | GPIO_PIN(17))

/* USB **********************************************************************/

/* Second (power only) USB port detection */

#define GPIO_EXT_USB (GPIO_INPUT | GPIO_PORT1 | GPIO_PIN(9))

/* GPIO character devices ***************************************************/

/* The /dev/gpioN numbering below is fixed - it is the ABI for the Dawn
 * descriptors:
 *
 *   /dev/gpio0  - SW1_ONOFF (in)   range switch 1 status
 *   /dev/gpio1  - SW2_ONOFF (in)   range switch 2 status
 *   /dev/gpio2  - SW3_ONOFF (in)   range switch 3 status
 *   /dev/gpio3  - SW4_ONOFF (in)   range switch 4 status
 *   /dev/gpio4  - VEXT_EN   (out)  ampere meter input switch
 *   /dev/gpio5  - VLDO_EN   (out)  source meter output switch
 *   /dev/gpio6  - ANA_EN    (out)  +8V/-2V analog rails enable
 *   /dev/gpio7  - REG_EN    (out)  SMU regulators enable
 *   /dev/gpio8  - VOUT_EN   (out)  power output switch
 *   /dev/gpio9  - CAL100K   (out)  100k calibration load
 *   /dev/gpio10 - CAL10K    (out)  10k calibration load
 *   /dev/gpio11 - CAL1K     (out)  1k calibration load
 *   /dev/gpio12 - CAL100    (out)  100R calibration load
 *   /dev/gpio13 - LP_EN     (out)  logic port level shifter enable
 *   /dev/gpio14 - LP_D0     (in)   logic port bit 0
 *   /dev/gpio15 - LP_D1     (in)   logic port bit 1
 *   /dev/gpio16 - LP_D2     (in)   logic port bit 2
 *   /dev/gpio17 - LP_D3     (in)   logic port bit 3
 *   /dev/gpio18 - LP_D4     (in)   logic port bit 4
 *   /dev/gpio19 - LP_D5     (in)   logic port bit 5
 *   /dev/gpio20 - LP_D6     (in)   logic port bit 6
 *   /dev/gpio21 - LP_D7     (in)   logic port bit 7
 *   /dev/gpio22 - EXT_USB   (in)   second USB port detection
 */

#define PPK2_GPIO_NINPUTS1  (4)   /* SW1..SW4 */
#define PPK2_GPIO_NOUTPUTS  (10)  /* enables + calibration loads + LP_EN */
#define PPK2_GPIO_NINPUTS2  (9)   /* LP_D0..LP_D7 + EXT_USB */

/****************************************************************************
 * Public Types
 ****************************************************************************/

/****************************************************************************
 * Public Data
 ****************************************************************************/

#ifndef __ASSEMBLY__

/****************************************************************************
 * Public Functions Prototypes
 ****************************************************************************/

/****************************************************************************
 * Name: nrf52_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 ****************************************************************************/

int nrf52_bringup(void);

/****************************************************************************
 * Name: nrf52_adc_setup
 *
 * Description:
 *   Initialize ADC driver.
 *
 ****************************************************************************/

#ifdef CONFIG_ADC
int nrf52_adc_setup(void);
#endif

/****************************************************************************
 * Name: nrf52_rgbled_setup
 *
 * Description:
 *   Initialize the RGB lightwell LEDs driver.
 *
 ****************************************************************************/

#ifdef CONFIG_RGBLED
int nrf52_rgbled_setup(void);
#endif

/****************************************************************************
 * Name: nrf52_mcp445x_initialize
 *
 * Description:
 *   Initialize the MCP4451 potentiometer and register the POT device.
 *
 ****************************************************************************/

#ifdef CONFIG_POT_MCP445X
int nrf52_mcp445x_initialize(void);
#endif

/****************************************************************************
 * Name: nrf52_gpiodev_initialize
 *
 * Description:
 *   Initialize GPIO devices with PPK2 control and status pins.
 *
 ****************************************************************************/

#ifdef CONFIG_PPK2_GPIO
int nrf52_gpiodev_initialize(void);
#endif

/****************************************************************************
 * Name: ppk2_usb_detach
 *
 * Description:
 *   Disconnect the USB D+ pull-up so the gadget stays invisible to the host
 *   until dawn_board_net_ready() re-attaches it (after dhcpd is up).
 *
 ****************************************************************************/

#ifdef CONFIG_NET_CDCECM
void ppk2_usb_detach(void);
#endif

#endif /* __ASSEMBLY__ */
#endif /* __BOARDS_ARM_NRF52_PPK2_SRC_PPK2_H */
