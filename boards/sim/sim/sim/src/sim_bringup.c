/*
 * boards/sim/sim/sim/src/sim_bringup.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <debug.h>
#include <unistd.h>
#include <nuttx/serial/pty.h>

#include <nuttx/fs/fs.h>

#include "sim_internal.h"
#include "sim.h"

#ifdef CONFIG_DAWN_FAKE_FILES
#  include "dawn/fake_files.h"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sim_bringup
 *
 * Description:
 *   Bring up simulated board features
 *
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
  /* Mount the procfs file system */

  ret = nx_mount(NULL, SIM_PROCFS_MOUNTPOINT, "procfs", 0, NULL);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount procfs at %s: %d\n",
             SIM_PROCFS_MOUNTPOINT, ret);
    }
#endif

#ifdef CONFIG_FS_HOSTFS
  /* Mount the host file system */

  ret = nx_mount(NULL, SIM_HOSTFS_MOUNTPOINT, "hostfs", 0,
                 SIM_HOSTFS_CONFIG);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to mount hostfs at %s: %d\n",
             SIM_PROCFS_MOUNTPOINT, ret);
    }
#endif

#ifdef CONFIG_DAWN_SIM_TTY_ALIAS
  /* Move the sim UART device out of /tmp before tmpfs is mounted there.
   * CONFIG_SIM_UART0_NAME is also the host PTY path used by sim_hostuart,
   * so the UART driver still opens the host-side /tmp/ttySIM0.
   */

  unlink("/dev/ttyS1");
  ret = rename(CONFIG_SIM_UART0_NAME, "/dev/ttyS1");
  if (ret < 0)
    {
      ret = symlink(CONFIG_SIM_UART0_NAME, "/dev/ttyS1");
      if (ret < 0)
        {
          syslog(LOG_ERR, "ERROR: tty alias /dev/ttyS1 -> %s failed: %d\n",
                 CONFIG_SIM_UART0_NAME, ret);
        }
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

#ifdef CONFIG_DAWN_FAKE_SENSORS
  /* Initialize fake sensors */

  ret = fake_sensors_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize sensors %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_IOEXPANDER
  /* Initialize fake GPIO */

  ret = fake_ioexpander_initialize();
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize gpio %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_ADC
  /* Initialize fake ADCs */

  ret = fake_adc_initialize(0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize ADC %d\n", ret);
    }

  ret = fake_adc_initialize(1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize ADC %d\n", ret);
    }

  ret = fake_adc_initialize(2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize ADC %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_DAC
  /* Initialize fake DACs */

  ret = fake_dac_initialize(0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize DAC %d\n", ret);
    }

  ret = fake_dac_initialize(1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize DAC %d\n", ret);
    }

  ret = fake_dac_initialize(2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize DAC %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_PWM
  /* Initialize fake PWMs */

  ret = fake_pwm_initialize(0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize PWM %d\n", ret);
    }

  ret = fake_pwm_initialize(1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize PWM %d\n", ret);
    }

  ret = fake_pwm_initialize(2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize PWM %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_PULSECOUNT
  /* Initialize fake pulsecount devices */

  ret = fake_pulsecount_initialize(0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize pulsecount %d\n", ret);
    }

  ret = fake_pulsecount_initialize(1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize pulsecount %d\n", ret);
    }

  ret = fake_pulsecount_initialize(2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize pulsecount %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_ENCODER
  /* Initialize fake encoders */

  ret = fake_encoder_initialize(0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize encoder %d\n", ret);
    }

  ret = fake_encoder_initialize(1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize encoder %d\n", ret);
    }

  ret = fake_encoder_initialize(2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize encoder %d\n", ret);
    }
#endif

#ifdef CONFIG_SIM_CANDEV_CHAR
  ret = sim_canchar_initialize(0, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: sim_canchar_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_BUTTONS
  ret = fake_buttons_initialize(0, 8);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_buttons_initialize() failed: %d\n", ret);
    }

  ret = fake_buttons_initialize(1, 8);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_buttons_initialize() failed: %d\n", ret);
    }

  ret = fake_buttons_initialize(2, 8);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_buttons_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_USERLEDS
  ret = fake_userleds_initialize(0, 8);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_userleds_initialize() failed: %d\n", ret);
    }

  ret = fake_userleds_initialize(1, 8);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_userleds_initialize() failed: %d\n", ret);
    }

  ret = fake_userleds_initialize(2, 8);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_userleds_initialize() failed: %d\n", ret);
    }
#endif

#ifdef CONFIG_DAWN_FAKE_RGBLED
  ret = fake_rgbled_initialize(0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_rgbled_initialize() failed: %d\n", ret);
    }

  ret = fake_rgbled_initialize(1);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_rgbled_initialize() failed: %d\n", ret);
    }

  ret = fake_rgbled_initialize(2);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: fake_rgbled_initialize() failed: %d\n", ret);
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

/* Note: CONFIG_DAWN_FAKE_POWEROFF is not used on simulator because
 * the sim architecture already provides board_power_off()
 */

#ifdef CONFIG_DAWN_FAKE_NET
  /* Initialize fake network stubs (forces linking of NX* functions) */

  fake_net_initialize();
#endif

  return ret;
}
