/****************************************************************************
 * boards/arm/nrf54l/nrf54lm20-dk/src/nrf54l_bringup.c
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

#include <syslog.h>

#include <nuttx/fs/fs.h>

#ifdef CONFIG_USERLED_LOWER
#  include <nuttx/leds/userled.h>
#endif

#ifdef CONFIG_DAWN_FAKE_FILES
#  include "dawn/fake_files.h"
#endif

#ifdef CONFIG_DAWN_FAKE_DRIVERS
#  include "dawn/fake_drivers.h"
#endif

#include "nrf54lm20-dk.h"

#ifdef CONFIG_NRF54L_SOFTDEVICE_CONTROLLER
#  include "nrf54l_sdc.h"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nrf54l_bringup
 *
 * Description:
 *   Mount optional file systems and register enabled board devices.
 *
 ****************************************************************************/

int nrf54l_bringup(void)
{
  int ret;

#ifdef CONFIG_FS_PROCFS
  /* Mount the procfs file system */

  ret = nx_mount(NULL, NRF54L_PROCFS_MOUNTPOINT, "procfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: procfs mount failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_USERLED_LOWER
  /* Register the LED driver */

  ret = userled_lower_initialize("/dev/userleds");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: LED registration failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_FS_TMPFS
  /* Mount the tmpfs file system */

  ret = nx_mount(NULL, CONFIG_LIBC_TMPDIR, "tmpfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount tmpfs at %s: %d\n",
             CONFIG_LIBC_TMPDIR, ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_FILES
  /* Pre-populate fake files in tmpfs (must run after the mount above) */

  ret = dawn_fake_files_init();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: dawn_fake_files_init() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_IOEXPANDER
  ret = fake_ioexpander_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_ioexpander_initialize() failed: %d\n",
             ret);
    }
#endif

#ifdef CONFIG_NRF54L_SOFTDEVICE_CONTROLLER
  /* Register the Bluetooth controller */

  ret = nrf54l_sdc_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: SDC initialization failed: %d\n", ret);
      return ret;
    }
#endif

  UNUSED(ret);
  return OK;
}
