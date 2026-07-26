/*
 * boards/arm/stm32f4/stm32f4discovery/src/stm32_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <debug.h>

#include <sys/types.h>

#include <nuttx/fs/fs.h>

#ifdef CONFIG_INPUT_BUTTONS
#  include <nuttx/input/buttons.h>
#endif

#ifdef CONFIG_USERLED
#  include <nuttx/leds/userled.h>
#endif

#if defined(CONFIG_CDCACM) && !defined(CONFIG_CDCACM_COMPOSITE)
#  include <nuttx/usb/cdcacm.h>
#endif

#if defined(CONFIG_NET_CDCECM) && !defined(CONFIG_CDCECM_COMPOSITE)
#  include <nuttx/usb/cdcecm.h>
#endif

#include <arch/board/board.h>

#include "stm32f4discovery.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: stm32_bringup
 *
 * Description:
 *   Perform architecture-specific initialization
 *
 ****************************************************************************/

int stm32_bringup(void)
{
  int ret;

#ifdef CONFIG_USERLED
  /* Register the LED driver */

  ret = userled_lower_initialize("/dev/leds0");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: userled_lower_initialize() failed: %d\n", ret);
      return ret;
    }
#endif

#ifdef CONFIG_INPUT_BUTTONS
  /* Register the BUTTON driver */

  ret = btn_lower_initialize("/dev/buttons0");
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: btn_lower_initialize() failed: %d\n", ret);
    }
#endif

#if defined(CONFIG_CDCACM) && !defined(CONFIG_CDCACM_COMPOSITE) && \
    !defined(CONFIG_NSH_USBCONSOLE)
  /* Register the CDC/ACM serial device (/dev/ttyACM0) used as the Dawn
   * protocol transport over USB.
   *
   * The device is removable: NuttX refuses open() until the USB host has
   * enumerated it (-ENOTCONN).  The Dawn serial / Modbus-RTU proto handles
   * this by retrying the open in its worker thread, so no board-level wait is
   * needed here.  When CONFIG_NSH_USBCONSOLE is used the CDC/ACM is the NSH
   * console and NSH connects it itself via boardctl(); do not init here.
   */

  ret = cdcacm_initialize(0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: cdcacm_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_SENSORS_LIS3DSH_UORB
  /* Register the on-board LIS3DSH accelerometer as a uORB sensor on SPI1 */

  ret = board_lis3dsh_initialize(0, 1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: board_lis3dsh_initialize() failed: %d\n", ret);
    }
#endif

#if defined(CONFIG_NET_CDCECM) && !defined(CONFIG_CDCECM_COMPOSITE)
  /* Register the CDC/ECM USB network device.  This creates the ethX network
   * interface used as the Dawn protocol transport for Modbus TCP and UDP.
   */

  ret = cdcecm_initialize(0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: cdcecm_initialize() failed: %d\n", ret);
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

  UNUSED(ret);
  return OK;
}
