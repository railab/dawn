/****************************************************************************
 * boards/arm/nrf52/ppk2/src/nrf52_eeprom.c
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

#include <errno.h>
#include <nuttx/debug.h>

#include <nuttx/i2c/i2c_master.h>
#include <nuttx/eeprom/eeprom.h>

#include "nrf52_i2c.h"
#include "ppk2.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* 24CW160 (2 KB): 0x000..0x0FC is Nordic factory calibration, never
 * overwrite; the Dawn calibration blob lives at 0x7C0.
 */

#define EEPROM_I2C_BUS (0)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nrf52_eeprom_init
 *
 * Description:
 *   Register the on-board 24CW160 I2C EEPROM as /dev/eeprom0.
 *
 ****************************************************************************/

int nrf52_eeprom_init(void)
{
  struct i2c_master_s *i2c;
  int                  ret;

  i2c = nrf52_i2cbus_initialize(EEPROM_I2C_BUS);
  if (i2c == NULL)
    {
      ferr("ERROR: Failed to initialize I2C%d\n", EEPROM_I2C_BUS);
      return -ENODEV;
    }

  ret = ee24xx_initialize(i2c, PPK2_EEPROM_ADDR, "/dev/eeprom0",
                          EEPROM_24CW160, 0);
  if (ret < 0)
    {
      ferr("ERROR: ee24xx_initialize failed: %d\n", ret);
      return ret;
    }

  finfo("24CW160 EEPROM registered as /dev/eeprom0\n");
  return OK;
}
