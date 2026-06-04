/****************************************************************************
 * boards/arm/nrf91/thingy91/src/nrf91_pmic.c
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
#include <stdint.h>

#include <nuttx/arch.h>
#include <nuttx/i2c/i2c_master.h>

#include "nrf91_i2c.h"
#include "thingy91.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The Thingy:91 ADP5360 PMIC sits on I2C2. Its buck-boost regulator powers
 * the 3.3 V rail (BH1749 colour sensor, RGB/sense LEDs, ...). The buck-boost
 * is disabled at power-on reset, so without this init the 3.3 V rail is dead
 * and those parts never respond (NCS enables it via its regulator driver).
 */

#define ADP5360_I2C_BUS         (2)
#define ADP5360_I2C_ADDR        (0x46)
#define ADP5360_I2C_FREQUENCY   (400000)

#define ADP5360_BUCKBST_CFG     (0x2b)  /* Buck-boost configure register */
#define ADP5360_BUCKBST_OUTPUT  (0x2c)  /* Buck-boost output voltage register */

#define ADP5360_BUCKBST_EN      (1 << 0) /* EN_BUCKBST bit in BUCKBST_CFG */
#define ADP5360_BUCKBST_3V3     (0x13)   /* 2.95V + 7*50mV = 3.3V */

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: adp5360_getreg8
 ****************************************************************************/

static int adp5360_getreg8(struct i2c_master_s *i2c, uint8_t reg,
                           uint8_t *val)
{
  struct i2c_msg_s msg[2];

  msg[0].frequency = ADP5360_I2C_FREQUENCY;
  msg[0].addr      = ADP5360_I2C_ADDR;
  msg[0].flags     = 0;
  msg[0].buffer    = &reg;
  msg[0].length    = 1;

  msg[1].frequency = ADP5360_I2C_FREQUENCY;
  msg[1].addr      = ADP5360_I2C_ADDR;
  msg[1].flags     = I2C_M_READ;
  msg[1].buffer    = val;
  msg[1].length    = 1;

  return I2C_TRANSFER(i2c, msg, 2);
}

/****************************************************************************
 * Name: adp5360_putreg8
 ****************************************************************************/

static int adp5360_putreg8(struct i2c_master_s *i2c, uint8_t reg,
                           uint8_t val)
{
  struct i2c_msg_s msg;
  uint8_t          buf[2];

  buf[0] = reg;
  buf[1] = val;

  msg.frequency = ADP5360_I2C_FREQUENCY;
  msg.addr      = ADP5360_I2C_ADDR;
  msg.flags     = 0;
  msg.buffer    = buf;
  msg.length    = 2;

  return I2C_TRANSFER(i2c, &msg, 1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nrf91_pmic_init
 *
 * Description:
 *   Bring up the Thingy:91 ADP5360 3.3 V buck-boost rail so that the 3.3 V
 *   peripherals (BH1749 colour sensor, RGB LEDs) are powered.
 *
 ****************************************************************************/

int nrf91_pmic_init(void)
{
  struct i2c_master_s *i2c;
  uint8_t              regval;
  int                  ret;

  i2c = nrf91_i2cbus_initialize(ADP5360_I2C_BUS);
  if (i2c == NULL)
    {
      return -ENODEV;
    }

  /* Make sure the buck-boost output is set to 3.3 V */

  ret = adp5360_putreg8(i2c, ADP5360_BUCKBST_OUTPUT, ADP5360_BUCKBST_3V3);
  if (ret < 0)
    {
      snerr("ERROR: ADP5360 set 3V3 failed: %d\n", ret);
      return ret;
    }

  /* Enable the buck-boost regulator (read-modify-write to keep other bits) */

  ret = adp5360_getreg8(i2c, ADP5360_BUCKBST_CFG, &regval);
  if (ret < 0)
    {
      snerr("ERROR: ADP5360 read BUCKBST_CFG failed: %d\n", ret);
      return ret;
    }

  regval |= ADP5360_BUCKBST_EN;

  ret = adp5360_putreg8(i2c, ADP5360_BUCKBST_CFG, regval);
  if (ret < 0)
    {
      snerr("ERROR: ADP5360 enable buck-boost failed: %d\n", ret);
      return ret;
    }

  /* Let the 3.3 V rail (and the parts it powers) settle before use */

  up_mdelay(20);

  sninfo("ADP5360 3.3V buck-boost enabled (BUCKBST_CFG=0x%02x)\n", regval);
  return OK;
}
