/****************************************************************************
 * boards/arm/nrf52/ppk2/src/nrf52_rgbled.c
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
#include <stddef.h>
#include <string.h>

#include <nuttx/timers/pwm.h>
#include <nuttx/leds/rgbled.h>
#include <arch/board/board.h>

#include "nrf52_pwm.h"

#include "ppk2.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/* Both RGB lightwell LEDs share the same nets driven from PWM0:
 *   CH0 - LEDR, CH1 - LEDG, CH2 - LEDB
 */

#define RGBLED_PWM       (0)
#define RGBLED_R_CHANNEL (1)
#define RGBLED_G_CHANNEL (2)
#define RGBLED_B_CHANNEL (3)

#if CONFIG_PWM_NCHANNELS < 3
#  error RGB LEDs require CONFIG_PWM_NCHANNELS >= 3
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nrf52_rgbled_setup
 *
 * Description:
 *   Initialize the RGB lightwell LEDs and register the RGB LED device.
 *
 ****************************************************************************/

int nrf52_rgbled_setup(void)
{
  static bool             initialized = false;
  struct pwm_lowerhalf_s *pwm         = NULL;
  struct pwm_info_s       info;
  int                     ret         = OK;

  /* Have we already initialized? */

  if (!initialized)
    {
      /* Call nrf52_pwminitialize() to get an instance of the PWM
       * interface
       */

      pwm = nrf52_pwminitialize(RGBLED_PWM);
      if (!pwm)
        {
          lederr("ERROR: Failed to get the NRF52 PWM lower half\n");
          return -ENODEV;
        }

      /* Setup PWM and start with all LEDs off */

      pwm->ops->setup(pwm);

      memset(&info, 0, sizeof(struct pwm_info_s));
      info.frequency = CONFIG_RGBLED_PWM_FREQ;
      info.channels[0].channel = RGBLED_R_CHANNEL;
      info.channels[1].channel = RGBLED_G_CHANNEL;
      info.channels[2].channel = RGBLED_B_CHANNEL;

      pwm->ops->start(pwm, &info);

      /* Register the RGB LED driver at "/dev/rgbled0" */

      ret = rgbled_register("/dev/rgbled0", pwm, pwm, pwm,
                            RGBLED_R_CHANNEL, RGBLED_G_CHANNEL,
                            RGBLED_B_CHANNEL);
      if (ret < 0)
        {
          lederr("ERROR: rgbled_register failed: %d\n", ret);
          return ret;
        }

      /* Now we are initialized */

      initialized = true;
    }

  return ret;
}
