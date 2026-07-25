/****************************************************************************
 * boards/arm/nrf52/ppk2/src/nrf52_bringup.c
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

#include <sys/types.h>
#include <syslog.h>

#include <nuttx/fs/fs.h>

#ifdef CONFIG_USBMONITOR
#  include <nuttx/usb/usbmonitor.h>
#endif

#ifdef CONFIG_USBDEV_COMPOSITE
#  include <nuttx/board.h>
/* Defined in nrf52_composite.c (board.h only declares these under
 * CONFIG_BOARDCTL_USBDEVCTRL, which we do not enable).
 */

int board_composite_initialize(int port);
FAR void *board_composite_connect(int port, int configid);
#endif

#ifdef CONFIG_RNDIS
#  include <nuttx/usb/rndis.h>
#endif

#ifdef CONFIG_NET_CDCNCM
#  include <nuttx/usb/cdcncm.h>
#endif

#ifdef CONFIG_NET_CDCECM
#  include <nuttx/usb/cdcecm.h>
#endif

#ifdef CONFIG_I2C_EE_24XX
#  include <nuttx/eeprom/eeprom.h>
#endif

#ifdef CONFIG_I2C
#  include "nrf52_i2c.h"
#endif

#ifdef CONFIG_NRF52_PROGMEM
#  include "nrf52_progmem.h"
#endif

#ifdef CONFIG_DAWN_FAKE_FILES
#  include "dawn/fake_files.h"
#endif

#ifdef CONFIG_DAWN_FAKE_DRIVERS
#  include "dawn/fake_drivers.h"
#endif

#include "ppk2.h"

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#define PPK2_EEPROM_I2CBUS (0)

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: nrf52_i2c_register
 *
 * Description:
 *   Register one I2C drivers for the I2C tool.
 *
 ****************************************************************************/

#if defined(CONFIG_I2C) && defined(CONFIG_SYSTEM_I2CTOOL)
static void nrf52_i2c_register(int bus)
{
  struct i2c_master_s *i2c;
  int ret;

  i2c = nrf52_i2cbus_initialize(bus);
  if (i2c == NULL)
    {
      syslog(LOG_ERR, "ERROR: Failed to get I2C%d interface\n", bus);
    }
  else
    {
      ret = i2c_register(i2c, bus);
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: Failed to register I2C%d driver: %d\n",
                 bus, ret);
          nrf52_i2cbus_uninitialize(i2c);
        }
    }
}
#endif

/****************************************************************************
 * Name: nrf52_i2ctool
 *
 * Description:
 *   Register I2C drivers for the I2C tool.
 *
 ****************************************************************************/

#if defined(CONFIG_I2C) && defined(CONFIG_SYSTEM_I2CTOOL)
static void nrf52_i2ctool(void)
{
#ifdef CONFIG_NRF52_I2C0_MASTER
  nrf52_i2c_register(0);
#endif
#ifdef CONFIG_NRF52_I2C1_MASTER
  nrf52_i2c_register(1);
#endif
}
#endif

/****************************************************************************
 * Name: nrf52_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 *   CONFIG_BOARD_LATE_INITIALIZE=y :
 *     Called from board_late_initialize().
 *
 ****************************************************************************/

int nrf52_bringup(void)
{
  int ret;

#if defined(CONFIG_I2C) && defined(CONFIG_SYSTEM_I2CTOOL)
  nrf52_i2ctool();
#endif

#ifdef CONFIG_FS_PROCFS
  /* Mount the procfs file system */

  ret = nx_mount(NULL, NRF52_PROCFS_MOUNTPOINT, "procfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: Failed to mount the PROC filesystem: %d\n",  ret);
    }
#endif /* CONFIG_FS_PROCFS */

#ifdef CONFIG_POT_MCP445X
  /* Initialize the MCP4451 potentiometer */

  ret = nrf52_mcp445x_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: nrf52_mcp445x_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_I2C_EE_24XX
  /* Register the calibration EEPROM. Bytes 0x000..0x0FC hold the Nordic
   * factory calibration and must never be overwritten (byte-exact backup
   * in ppk2-backup/); the self-calibration blob lives in the virgin tail
   * at 0x7C0 and the host tooling refuses writes anywhere else.
   */

  ret = ee24xx_initialize(nrf52_i2cbus_initialize(PPK2_EEPROM_I2CBUS),
                          PPK2_EEPROM_ADDR, "/dev/eeprom0",
                          EEPROM_24CW160, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: ee24xx_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_ADC
  /* Configure ADC driver */

  ret = nrf52_adc_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: Failed to initialize ADC driver: %d\n",
             ret);
    }
#endif

#ifdef CONFIG_RGBLED
  /* Configure RGB LED driver */

  ret = nrf52_rgbled_setup();
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: Failed to initialize RGB LED driver: %d\n",
             ret);
    }
#endif

#ifdef CONFIG_PPK2_GPIO
  ret = nrf52_gpiodev_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: nrf52_gpiodev_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_USBMONITOR
  /* Start the USB Monitor */

  ret = usbmonitor_start();
  if (ret != OK)
    {
      syslog(LOG_ERR, "ERROR: Failed to start USB monitor: %d\n", ret);
    }
#endif

#if defined(CONFIG_NET_CDCECM) && !defined(CONFIG_CDCECM_COMPOSITE)
  /* Register the CDC-ECM USB network device (standard ethernet-over-USB). */

  ret = cdcecm_initialize(0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: cdcecm_initialize() failed: %d\n", ret);
    }
  else
    {
      /* Keep the gadget detached until dhcpd is up (see dawn_board_net_ready
       * / ppk2_usb_detach). A CDC-ECM host runs DHCP only on the first
       * link-up, so it must not enumerate before the server is listening.
       */

      ppk2_usb_detach();
    }
#elif defined(CONFIG_NET_CDCNCM) && !defined(CONFIG_CDCNCM_COMPOSITE)
  /* Register the CDC-NCM USB network device (modern, high-throughput
   * ethernet-over-USB with NTB frame aggregation).
   */

  ret = cdcncm_initialize(0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: cdcncm_initialize() failed: %d\n", ret);
    }
#elif defined(CONFIG_RNDIS) && !defined(CONFIG_RNDIS_COMPOSITE)
  /* Register the RNDIS USB network device */

  {
    uint8_t mac[6];

    mac[0] = 0xa0;
    mac[1] = (CONFIG_NETINIT_MACADDR_2 >> (8 * 0)) & 0xff;
    mac[2] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 3)) & 0xff;
    mac[3] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 2)) & 0xff;
    mac[4] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 1)) & 0xff;
    mac[5] = (CONFIG_NETINIT_MACADDR_1 >> (8 * 0)) & 0xff;
    usbdev_rndis_initialize(mac);
  }
#endif

#if defined(CONFIG_USBDEV_COMPOSITE) && !defined(CONFIG_NSH_USBCONSOLE)
  /* Initialize and connect the composite USB device. With the NSH USB
   * console enabled this is left to nsh_consolemain() instead.
   */

  ret = board_composite_initialize(0);
  if (ret < 0)
    {
      syslog(LOG_ERR,
             "ERROR: board_composite_initialize() failed: %d\n", ret);
    }
  else if (board_composite_connect(0, 0) == NULL)
    {
      syslog(LOG_ERR, "ERROR: board_composite_connect() failed\n");
    }
#endif

#ifdef CONFIG_NRF52_PROGMEM
  ret = nrf52_progmem_init();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize MTD progmem: %d\n", ret);
    }
#endif /* CONFIG_MTD */

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

  UNUSED(ret);
  return OK;
}
