/*
 * examples/out-of-tree-demo/boards/sim/sim/sim/src/sim_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Minimal bringup for the Dawn out-of-tree demo board. Only mounts procfs
 * and initialises the small set of fake drivers the demo's defconfig.dawn
 * enables.
 */

#include <nuttx/config.h>

#include <debug.h>
#include <nuttx/serial/pty.h>
#include <nuttx/fs/fs.h>

#include "sim.h"

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int sim_bringup(void)
{
  int ret = OK;

#ifdef CONFIG_PSEUDOTERM
  ret = pty_register(0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to register pty: %d\n", ret);
    }
#endif

#ifdef CONFIG_FS_PROCFS
  ret = nx_mount(NULL, SIM_PROCFS_MOUNTPOINT, "procfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount procfs at %s: %d\n",
             SIM_PROCFS_MOUNTPOINT, ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_USERLEDS
  ret = fake_userleds_initialize(0, 8);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_userleds_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_BUTTONS
  ret = fake_buttons_initialize(0, 8);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_buttons_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_UID
  ret = fake_uid_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_uid_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_RESET
  fake_reset_initialize();
#endif

  return ret;
}
