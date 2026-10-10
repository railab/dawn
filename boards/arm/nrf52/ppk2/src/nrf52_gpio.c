/****************************************************************************
 * boards/arm/nrf52/ppk2/src/nrf52_gpio.c
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

#include <stdint.h>
#include <stdbool.h>
#include <errno.h>
#include <nuttx/debug.h>

#include <nuttx/ioexpander/gpio.h>
#include <nuttx/irq.h>

#include <arch/board/board.h>

#include "chip.h"
#include "arm_internal.h"
#include "nrf52_gpiote.h"
#include "ppk2.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct nrf52gpio_dev_s
{
  struct gpio_dev_s gpio;
  uint8_t           id;
};

struct nrf52gpint_dev_s
{
  struct nrf52gpio_dev_s nrf52gpio;
  pin_interrupt_t        callback;
  bool                   enabled;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int gpin1_read(struct gpio_dev_s *dev, bool *value);
static int nrf52gpio_interrupt(int irq, void *context, void *arg);
static int gpin1_attach(struct gpio_dev_s *dev, pin_interrupt_t callback);
static int gpin1_enable(struct gpio_dev_s *dev, bool enable);
static void gpin1_arm(struct nrf52gpint_dev_s *nrf52gpint);
static int gpin2_read(struct gpio_dev_s *dev, bool *value);
static int gpout_read(struct gpio_dev_s *dev, bool *value);
static int gpout_write(struct gpio_dev_s *dev, bool value);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct gpio_operations_s gpin1_ops =
{
  .go_read   = gpin1_read,
  .go_write  = NULL,
  .go_attach = gpin1_attach,
  .go_enable = gpin1_enable,
};

static const struct gpio_operations_s gpin2_ops =
{
  .go_read   = gpin2_read,
  .go_write  = NULL,
  .go_attach = NULL,
  .go_enable = NULL,
};

static const struct gpio_operations_s gpout_ops =
{
  .go_read   = gpout_read,
  .go_write  = gpout_write,
  .go_attach = NULL,
  .go_enable = NULL,
};

/* Range switch status as interrupt-capable inputs (GPIOTE, both edges) */

static const uint32_t g_gpioinputs1[PPK2_GPIO_NINPUTS1] =
{
  GPIO_SW1_ONOFF,
  GPIO_SW2_ONOFF,
  GPIO_SW3_ONOFF,
  GPIO_SW4_ONOFF,
};

static struct nrf52gpint_dev_s g_gpin1[PPK2_GPIO_NINPUTS1];

/* Analog rails state: unpowered range comparators toggle freely */

static bool g_rails_on;

/* Power path control, calibration loads and logic port enable as outputs */

static const uint32_t g_gpiooutputs[PPK2_GPIO_NOUTPUTS] =
{
  GPIO_VEXT_EN,
  GPIO_VLDO_EN,
  GPIO_ANA_EN,
  GPIO_REG_EN,
  GPIO_VOUT_EN,
  GPIO_CAL100K,
  GPIO_CAL10K,
  GPIO_CAL1K,
  GPIO_CAL100,
  GPIO_LP_EN,
};

static struct nrf52gpio_dev_s g_gpout[PPK2_GPIO_NOUTPUTS];

/* VEXT_EN, VLDO_EN and VOUT_EN are active-low P-FET power path switches;
 * the character devices always use 1 = "switch closed".
 */

static const bool g_gpiooutinvert[PPK2_GPIO_NOUTPUTS] =
{
  true,                       /* VEXT_EN */
  true,                       /* VLDO_EN */
  false,                      /* ANA_EN */
  false,                      /* REG_EN */
  true,                       /* VOUT_EN */
  false,                      /* CAL100K */
  false,                      /* CAL10K */
  false,                      /* CAL1K */
  false,                      /* CAL100 */
  false,                      /* LP_EN */
};

/* Logic port data and USB detection as inputs */

static const uint32_t g_gpioinputs2[PPK2_GPIO_NINPUTS2] =
{
  GPIO_LP_D0,
  GPIO_LP_D1,
  GPIO_LP_D2,
  GPIO_LP_D3,
  GPIO_LP_D4,
  GPIO_LP_D5,
  GPIO_LP_D6,
  GPIO_LP_D7,
  GPIO_EXT_USB,
};

static struct nrf52gpio_dev_s g_gpin2[PPK2_GPIO_NINPUTS2];

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int gpin1_read(struct gpio_dev_s *dev, bool *value)
{
  struct nrf52gpio_dev_s *io = (struct nrf52gpio_dev_s *)dev;

  *value = nrf52_gpio_read(g_gpioinputs1[io->id]);
  return OK;
}

/* Comparator outputs, clean edges.  GPIOTE channel = pin id (0..3), fixed,
 * so pins never share or steal channels (SPI workaround uses ch 7).
 */

static int nrf52gpio_interrupt(int irq, void *context, void *arg)
{
  struct nrf52gpint_dev_s *nrf52gpint = (struct nrf52gpint_dev_s *)arg;

  DEBUGASSERT(nrf52gpint != NULL && nrf52gpint->callback != NULL);

  nrf52gpint->callback(&nrf52gpint->nrf52gpio.gpio,
                       nrf52gpint->nrf52gpio.id);
  return OK;
}

static int gpin1_attach(struct gpio_dev_s *dev, pin_interrupt_t callback)
{
  struct nrf52gpint_dev_s *nrf52gpint = (struct nrf52gpint_dev_s *)dev;
  uint8_t id = nrf52gpint->nrf52gpio.id;

  /* Make sure the interrupt is disabled */

  nrf52_gpiote_set_ch_event(g_gpioinputs1[id], id, false, false,
                            NULL, NULL);

  nrf52gpint->callback = callback;
  return OK;
}

static void gpin1_arm(struct nrf52gpint_dev_s *nrf52gpint)
{
  uint8_t id = nrf52gpint->nrf52gpio.id;

  /* Both edges, only while the rails are up: with the rails off the
   * comparators toggle freely and the edge rate livelocks the CPU.
   */

  if (nrf52gpint->enabled && g_rails_on)
    {
      nrf52_gpiote_set_ch_event(g_gpioinputs1[id], id, true, true,
                                nrf52gpio_interrupt, nrf52gpint);
    }
  else
    {
      nrf52_gpiote_set_ch_event(g_gpioinputs1[id], id, false, false,
                                NULL, NULL);
    }
}

static int gpin1_enable(struct gpio_dev_s *dev, bool enable)
{
  struct nrf52gpint_dev_s *nrf52gpint = (struct nrf52gpint_dev_s *)dev;
  irqstate_t flags;

  if (enable && nrf52gpint->callback == NULL)
    {
      return -EINVAL;
    }

  flags = enter_critical_section();
  nrf52gpint->enabled = enable;
  gpin1_arm(nrf52gpint);
  leave_critical_section(flags);
  return OK;
}

static int gpin2_read(struct gpio_dev_s *dev, bool *value)
{
  struct nrf52gpio_dev_s *io = (struct nrf52gpio_dev_s *)dev;

  *value = nrf52_gpio_read(g_gpioinputs2[io->id]);
  return OK;
}

static int gpout_read(struct gpio_dev_s *dev, bool *value)
{
  struct nrf52gpio_dev_s *io = (struct nrf52gpio_dev_s *)dev;

  *value = nrf52_gpio_read(g_gpiooutputs[io->id]) ^
           g_gpiooutinvert[io->id];
  return OK;
}

static int gpout_write(struct gpio_dev_s *dev, bool value)
{
  struct nrf52gpio_dev_s *io = (struct nrf52gpio_dev_s *)dev;
  irqstate_t flags;
  int i;

  nrf52_gpio_write(g_gpiooutputs[io->id], value ^ g_gpiooutinvert[io->id]);

  if (io->id != PPK2_GPOUT_ANA_EN || value == g_rails_on)
    {
      return OK;
    }

  flags = enter_critical_section();
  g_rails_on = value;
  for (i = 0; i < PPK2_GPIO_NINPUTS1; i++)
    {
      gpin1_arm(&g_gpin1[i]);
    }

  leave_critical_section(flags);

  /* Edges were masked: notify once so the range state re-syncs */

  for (i = 0; value && i < PPK2_GPIO_NINPUTS1; i++)
    {
      if (g_gpin1[i].enabled)
        {
          g_gpin1[i].callback(&g_gpin1[i].nrf52gpio.gpio, i);
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nrf52_gpiodev_initialize
 *
 * Description:
 *   Initialize GPIO devices with PPK2 control and status pins.
 *
 ****************************************************************************/

int nrf52_gpiodev_initialize(void)
{
  int i;
  int pincount = 0;

  /* Range switch status inputs (interrupt-capable) */

  for (i = 0; i < PPK2_GPIO_NINPUTS1; i++)
    {
      /* Setup and register the GPIO pin */

      g_gpin1[i].nrf52gpio.gpio.gp_pintype = GPIO_INTERRUPT_PIN;
      g_gpin1[i].nrf52gpio.gpio.gp_ops     = &gpin1_ops;
      g_gpin1[i].nrf52gpio.id              = i;
      g_gpin1[i].callback                  = NULL;
      g_gpin1[i].enabled                   = false;
      gpio_pin_register(&g_gpin1[i].nrf52gpio.gpio, pincount);

      /* Configure the pin that will be used as input */

      nrf52_gpio_config(g_gpioinputs1[i]);

      pincount++;
    }

  /* Outputs */

  for (i = 0; i < PPK2_GPIO_NOUTPUTS; i++)
    {
      /* Setup and register the GPIO pin */

      g_gpout[i].gpio.gp_pintype = GPIO_OUTPUT_PIN;
      g_gpout[i].gpio.gp_ops     = &gpout_ops;
      g_gpout[i].id              = i;
      gpio_pin_register(&g_gpout[i].gpio, pincount);

      /* Configure the pin that will be used as output */

      nrf52_gpio_config(g_gpiooutputs[i]);

      pincount++;
    }

  /* Logic port and USB detection inputs */

  for (i = 0; i < PPK2_GPIO_NINPUTS2; i++)
    {
      /* Setup and register the GPIO pin */

      g_gpin2[i].gpio.gp_pintype = GPIO_INPUT_PIN;
      g_gpin2[i].gpio.gp_ops     = &gpin2_ops;
      g_gpin2[i].id              = i;
      gpio_pin_register(&g_gpin2[i].gpio, pincount);

      /* Configure the pin that will be used as input */

      nrf52_gpio_config(g_gpioinputs2[i]);

      pincount++;
    }

  return OK;
}
