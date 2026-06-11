/****************************************************************************
 * boards/arm/nrf91/thingy91/src/nrf91_eeprom.c
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

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <debug.h>
#include <errno.h>

#include <nuttx/i2c/i2c_master.h>
#include <nuttx/eeprom/eeprom.h>

#include "nrf91_i2c.h"
#include "thingy91.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The Thingy:91 carries a Microchip 24CW160 (16-Kbit / 2 KB) I2C EEPROM on
 * I2C2 at address 0x50. Its main array is geometry-compatible with the
 * generic 24xx16 part (2048 bytes, 16-byte page, single-byte word address
 * with the three block-select bits folded into the I2C address). Not used by
 * Dawn yet; this just exposes it as /dev/eeprom0 for later use.
 */

#define EEPROM_I2C_BUS  (2)
#define EEPROM_I2C_ADDR (0x50)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nrf91_eeprom_init
 *
 * Description:
 *   Register the on-board 24CW160 I2C EEPROM as /dev/eeprom0.
 *
 ****************************************************************************/

int nrf91_eeprom_init(void)
{
  struct i2c_master_s *i2c;
  int                  ret;

  i2c = nrf91_i2cbus_initialize(EEPROM_I2C_BUS);
  if (i2c == NULL)
    {
      return -ENODEV;
    }

  ret = ee24xx_initialize(i2c, EEPROM_I2C_ADDR, "/dev/eeprom0",
                          EEPROM_24XX16, 0);
  if (ret < 0)
    {
      snerr("ERROR: ee24xx_initialize failed: %d\n", ret);
      return ret;
    }

  sninfo("24CW160 EEPROM registered as /dev/eeprom0\n");
  return OK;
}
