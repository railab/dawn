/*
 * boards/common/src/fake_pwm.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <stdio.h>
#include <debug.h>
#include <assert.h>

#include <nuttx/kmalloc.h>
#include <nuttx/timers/pwm.h>

/* No board-specific header needed */

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fake_pwm_s
{
  const struct pwm_ops_s *ops;   /* PWM operations */
  int                     devno; /* Device number */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int sim_pwm_setup(struct pwm_lowerhalf_s *dev);
static int sim_pwm_shutdown(struct pwm_lowerhalf_s *dev);
#ifdef CONFIG_PWM_PULSECOUNT
static int sim_pwm_start(struct pwm_lowerhalf_s *dev,
                         const struct pwm_info_s *info,
                         void *handle);
#else
static int sim_pwm_start(struct pwm_lowerhalf_s *dev,
                         const struct pwm_info_s *info);
#endif
static int sim_pwm_stop(struct pwm_lowerhalf_s *dev);
static int sim_pwm_ioctl(struct pwm_lowerhalf_s *dev, int cmd,
                         unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* This is the list of lower half PWM driver methods used by the upper half
 * driver.
 */

static const struct pwm_ops_s g_fake_pwmops =
{
  .setup       = sim_pwm_setup,
  .shutdown    = sim_pwm_shutdown,
  .start       = sim_pwm_start,
  .stop        = sim_pwm_stop,
  .ioctl       = sim_pwm_ioctl,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sim_pwm_setup
 *
 * Description:
 *   This method is called when the driver is opened.  The lower half driver
 *   should configure and initialize the device so that it is ready for use.
 *   It should not, however, output pulses until the start method is called.
 *
 ****************************************************************************/

static int sim_pwm_setup(struct pwm_lowerhalf_s *dev)
{
  return OK;
}

/****************************************************************************
 * Name: sim_pwm_shutdown
 *
 * Description:
 *   This method is called when the driver is closed.  The lower half driver
 *   stop pulsed output, free any resources, disable the timer hardware, and
 *   put the system into the lowest possible power usage state
 *
 ****************************************************************************/

static int sim_pwm_shutdown(struct pwm_lowerhalf_s *dev)
{
  return OK;
}

/****************************************************************************
 * Name: sim_pwm_start
 *
 * Description:
 *   (Re-)initialize the PWM and start the pulsed output
 *
 ****************************************************************************/

#ifdef CONFIG_PWM_PULSECOUNT
static int sim_pwm_start(struct pwm_lowerhalf_s *dev,
                         const struct pwm_info_s *info,
                         void *handle)
{
#error Not supported
}
#else
static int sim_pwm_start(struct pwm_lowerhalf_s *dev,
                         const struct pwm_info_s *info)
{
  pwminfo("START\n");

  return OK;
}
#endif

/****************************************************************************
 * Name: sim_pwm_stop
 *
 * Description:
 *   Stop the PWM
 *
 ****************************************************************************/

static int sim_pwm_stop(struct pwm_lowerhalf_s *dev)
{
  pwminfo("STOP\n");
  return OK;
}

/****************************************************************************
 * Name: sim_pwm_ioctl
 *
 * Description:
 *   Lower-half logic may support platform-specific ioctl commands
 *
 ****************************************************************************/

static int sim_pwm_ioctl(struct pwm_lowerhalf_s *dev, int cmd,
                         unsigned long arg)
{
  struct fake_pwm_s *priv = (struct fake_pwm_s *)dev;

  DEBUGASSERT(dev);

  /* There are no platform-specific ioctl commands */

  UNUSED(priv);

  return -ENOTTY;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: sim_pwm_initialie
 *
 * Description:
 *   Initialize simulated ADC.
 *
 ****************************************************************************/

int fake_pwm_initialize(int pwm)
{
  struct fake_pwm_s *lower = NULL;
  char              path[32];

  /* Alloc driver data */

  lower = kmm_zalloc(sizeof(struct fake_pwm_s));
  if (lower < 0)
    {
      pwmerr("ERROR: malloc failed\n");
      return -ENOMEM;
    }

  /* Init data */

  lower->ops   = &g_fake_pwmops;
  lower->devno = pwm;

  /* Get path */

  snprintf(path, PATH_MAX, "/dev/pwm%d", pwm);

  return pwm_register(path, (struct pwm_lowerhalf_s *)lower);
}
