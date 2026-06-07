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
#include <stdbool.h>
#include <syslog.h>

#include <nuttx/arch.h>
#include <nuttx/i2c/i2c_master.h>
#ifdef CONFIG_BATTERY_GAUGE
#  include <nuttx/power/battery_gauge.h>
#  include <nuttx/power/battery_ioctl.h>
#endif

#include "nrf91_i2c.h"
#include "thingy91.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* The Thingy:91 ADP5360 PMIC sits on I2C2. Its buck-boost regulator powers
 * the 3.3 V rail (BH1749 colour sensor, RGB/sense LEDs, ...). The buck-boost
 * is disabled at power-on reset, so without this init the 3.3 V rail is dead
 * and those parts never respond (NCS enables it via its regulator driver).
 *
 * This init also configures and enables the battery charger (VBUS input
 * current limit, charge current, over-charge protection, EN_CHG) and turns on
 * the fuel gauge.
 */

#define ADP5360_I2C_BUS         (2)
#define ADP5360_I2C_ADDR        (0x46)
#define ADP5360_I2C_FREQUENCY   (400000)

#define ADP5360_BUCKBST_CFG     (0x2b)  /* Buck-boost configure register */
#define ADP5360_BUCKBST_OUTPUT  (0x2c)  /* Buck-boost output voltage register */

#define ADP5360_BUCKBST_EN      (1 << 0) /* EN_BUCKBST bit in BUCKBST_CFG */
#define ADP5360_BUCKBST_3V3     (0x13)   /* 2.95V + 7*50mV = 3.3V */

/* Charger configuration registers */

#define ADP5360_CHG_VBUS_ILIM   (0x02)  /* Charger VBUS input current limit */
#define ADP5360_CHG_CURRENT_SET (0x04)  /* Charger current setting */
#define ADP5360_CHG_FUNC        (0x07)  /* Charger functional settings */
#define ADP5360_CHG_STATUS_1    (0x08)  /* Charger status 1 (charge state) */
#define ADP5360_BAT_OC_CHG      (0x15)  /* Battery over-charge current */
#define ADP5360_BAT_SOC         (0x21)  /* Battery state of charge (%) */
#define ADP5360_VBAT_READ_H     (0x25)  /* Battery voltage MSB */
#define ADP5360_VBAT_READ_L     (0x26)  /* Battery voltage LSB */
#define ADP5360_FUEL_GAUGE_MODE (0x27)  /* Fuel gauge mode / enable */

#define ADP5360_VBUS_ILIM_MSK   (0x07)        /* ILIM field, bits[2:0] */
#define ADP5360_VBUS_ILIM_500MA (0x07)        /* 500 mA input current limit */

#define ADP5360_CHG_ICHG_MSK    (0x1f)        /* ICHG field, bits[4:0] */
#define ADP5360_CHG_ICHG_320MA  (0x1f)        /* 320 mA charge current */

#define ADP5360_OC_CHG_MSK      (0x07 << 5)   /* OC_CHG field, bits[7:5] */
#define ADP5360_OC_CHG_400MA    (0x07 << 5)   /* 400 mA over-charge threshold */

#define ADP5360_CHG_FUNC_EN_CHG (1 << 0)      /* EN_CHG bit in CHG_FUNC */

#define ADP5360_FG_EN           (1 << 0)      /* EN_FG bit in FUEL_GAUGE_MODE */
#define ADP5360_FG_MODE_SLEEP   (1 << 1)      /* FG_MODE bit: 1 = sleep mode */

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
 * Name: adp5360_modifyreg8
 *
 * Description:
 *   Read-modify-write a single 8-bit register: clear the bits in 'clrbits'
 *   then set the bits in 'setbits', preserving all other bits.
 *
 ****************************************************************************/

static int adp5360_modifyreg8(struct i2c_master_s *i2c, uint8_t reg,
                              uint8_t clrbits, uint8_t setbits)
{
  uint8_t regval;
  int     ret;

  ret = adp5360_getreg8(i2c, reg, &regval);
  if (ret < 0)
    {
      return ret;
    }

  regval &= ~clrbits;
  regval |= setbits;

  return adp5360_putreg8(i2c, reg, regval);
}

/****************************************************************************
 * Name: adp5360_charging_init
 *
 * Description:
 *   Configure and enable the ADP5360 battery charger and fuel gauge:
 *   VBUS input current limit 500 mA, charge current 320 mA,
 *   over-charge protection 400 mA, EN_CHG, and fuel gauge enabled in sleep
 *   mode.
 *
 ****************************************************************************/

static int adp5360_charging_init(struct i2c_master_s *i2c)
{
  uint8_t func   = 0;
  uint8_t status = 0;
  uint8_t soc    = 0;
  uint8_t vbat_h = 0;
  uint8_t vbat_l = 0;
  unsigned int mv = 0;
  int ret;

  /* VBUS input current limit -> 500 mA */

  ret = adp5360_modifyreg8(i2c, ADP5360_CHG_VBUS_ILIM,
                           ADP5360_VBUS_ILIM_MSK, ADP5360_VBUS_ILIM_500MA);
  if (ret < 0)
    {
      snerr("ERROR: ADP5360 set VBUS ilim failed: %d\n", ret);
      return ret;
    }

  /* Charge current -> 320 mA */

  ret = adp5360_modifyreg8(i2c, ADP5360_CHG_CURRENT_SET,
                           ADP5360_CHG_ICHG_MSK, ADP5360_CHG_ICHG_320MA);
  if (ret < 0)
    {
      snerr("ERROR: ADP5360 set charge current failed: %d\n", ret);
      return ret;
    }

  /* Over-charge protection threshold -> 400 mA */

  ret = adp5360_modifyreg8(i2c, ADP5360_BAT_OC_CHG,
                           ADP5360_OC_CHG_MSK, ADP5360_OC_CHG_400MA);
  if (ret < 0)
    {
      snerr("ERROR: ADP5360 set OC charge failed: %d\n", ret);
      return ret;
    }

  /* Enable the charger (EN_CHG) */

  ret = adp5360_modifyreg8(i2c, ADP5360_CHG_FUNC,
                           ADP5360_CHG_FUNC_EN_CHG, ADP5360_CHG_FUNC_EN_CHG);
  if (ret < 0)
    {
      snerr("ERROR: ADP5360 enable charging failed: %d\n", ret);
      return ret;
    }

  /* Enable the fuel gauge (sleep mode) so VBAT/SoC registers update */

  ret = adp5360_modifyreg8(i2c, ADP5360_FUEL_GAUGE_MODE,
                           ADP5360_FG_EN | ADP5360_FG_MODE_SLEEP,
                           ADP5360_FG_EN | ADP5360_FG_MODE_SLEEP);
  if (ret < 0)
    {
      snerr("ERROR: ADP5360 enable fuel gauge failed: %d\n", ret);
      return ret;
    }

  /* Read the charger state back and log it. */

  adp5360_getreg8(i2c, ADP5360_CHG_FUNC, &func);
  adp5360_getreg8(i2c, ADP5360_CHG_STATUS_1, &status);
  adp5360_getreg8(i2c, ADP5360_BAT_SOC, &soc);
  adp5360_getreg8(i2c, ADP5360_VBAT_READ_H, &vbat_h);
  adp5360_getreg8(i2c, ADP5360_VBAT_READ_L, &vbat_l);

  mv = ((unsigned int)vbat_h << 5) | ((unsigned int)vbat_l >> 3);

  syslog(LOG_INFO,
         "ADP5360 charging: CHG_FUNC=0x%02x (EN_CHG=%d) STATUS1=0x%02x "
         "(chg=%d) SoC=%u%% VBAT=%umV\n",
         func, func & ADP5360_CHG_FUNC_EN_CHG, status, status & 0x07,
         soc & 0x7f, mv);

  return OK;
}

#ifdef CONFIG_BATTERY_GAUGE

/****************************************************************************
 * ADP5360 battery_gauge lower-half (interim; to be extracted into
 * drivers/power/battery/adp5360.c). Exposes VBAT, SoC and charge state via
 * /dev/batt0 using the standard NuttX battery_gauge framework. Reads the
 * ADP5360 registers directly over I2C on each ioctl (the PMIC fuel gauge
 * refreshes them on its own), so no polling worker is needed for on-demand
 * LwM2M reads.
 ****************************************************************************/

struct adp5360_gauge_dev_s
{
  struct battery_gauge_dev_s dev;  /* Upper-half visible part (must be first) */
  struct i2c_master_s       *i2c;  /* I2C bus handle */
};

/****************************************************************************
 * Name: adp5360_gauge_state
 ****************************************************************************/

static int adp5360_gauge_state(struct battery_gauge_dev_s *dev, int *status)
{
  struct adp5360_gauge_dev_s *priv = (struct adp5360_gauge_dev_s *)dev;
  uint8_t regval;
  int     ret;

  ret = adp5360_getreg8(priv->i2c, ADP5360_CHG_STATUS_1, &regval);
  if (ret < 0)
    {
      *status = BATTERY_UNKNOWN;
      return ret;
    }

  /* CHG_STATUS_1 bits[2:0]: 0=off 1=trickle 2=fast-CC 3=fast-CV 4=complete */

  switch (regval & 0x07)
    {
      case 1:
      case 2:
      case 3:
        *status = BATTERY_CHARGING;
        break;
      case 4:
        *status = BATTERY_FULL;
        break;
      default:
        *status = BATTERY_IDLE;
        break;
    }

  return OK;
}

/****************************************************************************
 * Name: adp5360_gauge_online
 ****************************************************************************/

static int adp5360_gauge_online(struct battery_gauge_dev_s *dev, bool *status)
{
  *status = true;
  return OK;
}

/****************************************************************************
 * Name: adp5360_gauge_voltage
 *
 * Description:
 *   Battery voltage in mV (BATIOC_VOLTAGE convention).
 *
 ****************************************************************************/

static int adp5360_gauge_voltage(struct battery_gauge_dev_s *dev, int *value)
{
  struct adp5360_gauge_dev_s *priv = (struct adp5360_gauge_dev_s *)dev;
  uint8_t      msb;
  uint8_t      lsb;
  unsigned int mv;
  int          ret;

  ret = adp5360_getreg8(priv->i2c, ADP5360_VBAT_READ_H, &msb);
  if (ret < 0)
    {
      return ret;
    }

  ret = adp5360_getreg8(priv->i2c, ADP5360_VBAT_READ_L, &lsb);
  if (ret < 0)
    {
      return ret;
    }

  /* 13-bit value, 1 LSB = 1 mV. BATIOC_VOLTAGE returns mV directly. */

  mv = ((unsigned int)msb << 5) | ((unsigned int)lsb >> 3);

  *value = (int)mv;
  return OK;
}

/****************************************************************************
 * Name: adp5360_gauge_capacity
 *
 * Description:
 *   State of charge as an integer percentage (BATIOC_CAPACITY convention).
 *
 ****************************************************************************/

static int adp5360_gauge_capacity(struct battery_gauge_dev_s *dev, int *value)
{
  struct adp5360_gauge_dev_s *priv = (struct adp5360_gauge_dev_s *)dev;
  uint8_t regval;
  int     ret;

  ret = adp5360_getreg8(priv->i2c, ADP5360_BAT_SOC, &regval);
  if (ret < 0)
    {
      return ret;
    }

  *value = (int)(regval & 0x7f);
  return OK;
}

static const struct battery_gauge_operations_s g_adp5360_gauge_ops =
{
  adp5360_gauge_state,
  adp5360_gauge_online,
  adp5360_gauge_voltage,
  adp5360_gauge_capacity,
};

static struct adp5360_gauge_dev_s g_adp5360_gauge;

/****************************************************************************
 * Name: nrf91_battery_init
 *
 * Description:
 *   Register the ADP5360 fuel gauge as /dev/batt0.
 *
 ****************************************************************************/

int nrf91_battery_init(void)
{
  struct i2c_master_s *i2c;
  int                  ret;

  i2c = nrf91_i2cbus_initialize(ADP5360_I2C_BUS);
  if (i2c == NULL)
    {
      return -ENODEV;
    }

  g_adp5360_gauge.dev.ops = &g_adp5360_gauge_ops;
  g_adp5360_gauge.i2c     = i2c;

  ret = battery_gauge_register("/dev/batt0", &g_adp5360_gauge.dev);
  if (ret < 0)
    {
      snerr("ERROR: battery_gauge_register failed: %d\n", ret);
      return ret;
    }

  sninfo("ADP5360 fuel gauge registered as /dev/batt0\n");
  return OK;
}

#endif /* CONFIG_BATTERY_GAUGE */

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

  /* Configure and enable battery charging + fuel gauge */

  ret = adp5360_charging_init(i2c);
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}
