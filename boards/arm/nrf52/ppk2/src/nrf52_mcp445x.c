/****************************************************************************
 * boards/arm/nrf52/ppk2/src/nrf52_mcp445x.c
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

#include <nuttx/analog/pot.h>
#include <nuttx/i2c/i2c_master.h>

#include "nrf52_i2c.h"

#include "ppk2.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define MCP4451_I2CBUS (0)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nrf52_mcp445x_initialize
 *
 * Description:
 *   Initialize the MCP4451 potentiometer and register the POT device.
 *
 *   The MCP4451 wipers control (verified on hardware):
 *     wiper 0 - BB_ADJUST (VBB buck-boost output voltage)
 *     wiper 1 - LDO_ADJUST (VLDO output voltage)
 *     wiper 2 - unused (POT2 test point)
 *     wiper 3 - IA_OFFSET (in-amp offset, VREF_IA)
 *
 ****************************************************************************/

int nrf52_mcp445x_initialize(void)
{
  struct i2c_master_s *i2c = NULL;
  struct pot_dev_s    *pot = NULL;
  int                  ret = OK;

  /* Get the I2C bus instance */

  i2c = nrf52_i2cbus_initialize(MCP4451_I2CBUS);
  if (i2c == NULL)
    {
      aerr("ERROR: Failed to get I2C%d interface\n", MCP4451_I2CBUS);
      return -ENODEV;
    }

  /* Get an instance of the MCP4451 potentiometer */

  /* MCP4451-104: Rab = 100k */

  pot = mcp445x_initialize(i2c, PPK2_MCP4451_ADDR, 100000);
  if (pot == NULL)
    {
      aerr("ERROR: Failed to get the MCP445X lower half\n");
      return -ENODEV;
    }

  /* Register the POT driver at "/dev/pot0" */

  ret = pot_register("/dev/pot0", pot);
  if (ret < 0)
    {
      aerr("ERROR: pot_register failed: %d\n", ret);
    }

  return ret;
}
