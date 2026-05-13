/*
 * boards/common/src/fake_rgbled.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <debug.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include <nuttx/kmalloc.h>
#include <nuttx/leds/rgbled.h>
#include <nuttx/timers/pwm.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fake_rgbled_pwm_s
{
  const struct pwm_ops_s *ops;
  struct pwm_info_s info;
  bool started;
};

struct fake_rgbled_dev_s
{
  struct fake_rgbled_pwm_s red;
  struct fake_rgbled_pwm_s green;
  struct fake_rgbled_pwm_s blue;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int fake_rgbled_pwm_setup(struct pwm_lowerhalf_s *dev);
static int fake_rgbled_pwm_shutdown(struct pwm_lowerhalf_s *dev);
static int fake_rgbled_pwm_start(struct pwm_lowerhalf_s *dev, const struct pwm_info_s *info);
static int fake_rgbled_pwm_stop(struct pwm_lowerhalf_s *dev);
static int fake_rgbled_pwm_ioctl(struct pwm_lowerhalf_s *dev, int cmd, unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct pwm_ops_s g_fake_rgbled_pwm_ops = {
  .setup = fake_rgbled_pwm_setup,
  .shutdown = fake_rgbled_pwm_shutdown,
  .start = fake_rgbled_pwm_start,
  .stop = fake_rgbled_pwm_stop,
  .ioctl = fake_rgbled_pwm_ioctl,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fake_rgbled_pwm_setup
 ****************************************************************************/

static int fake_rgbled_pwm_setup(struct pwm_lowerhalf_s *dev)
{
  return OK;
}

/****************************************************************************
 * Name: fake_rgbled_pwm_shutdown
 ****************************************************************************/

static int fake_rgbled_pwm_shutdown(struct pwm_lowerhalf_s *dev)
{
  return OK;
}

/****************************************************************************
 * Name: fake_rgbled_pwm_start
 ****************************************************************************/

static int fake_rgbled_pwm_start(struct pwm_lowerhalf_s *dev, const struct pwm_info_s *info)
{
  struct fake_rgbled_pwm_s *priv = (struct fake_rgbled_pwm_s *)dev;

  if (priv == NULL || info == NULL)
    {
      return -EINVAL;
    }

  memcpy(&priv->info, info, sizeof(priv->info));
  priv->started = true;

  return OK;
}

/****************************************************************************
 * Name: fake_rgbled_pwm_stop
 ****************************************************************************/

static int fake_rgbled_pwm_stop(struct pwm_lowerhalf_s *dev)
{
  struct fake_rgbled_pwm_s *priv = (struct fake_rgbled_pwm_s *)dev;

  if (priv == NULL)
    {
      return -EINVAL;
    }

  priv->started = false;
  return OK;
}

/****************************************************************************
 * Name: fake_rgbled_pwm_ioctl
 ****************************************************************************/

static int fake_rgbled_pwm_ioctl(struct pwm_lowerhalf_s *dev, int cmd, unsigned long arg)
{
  return -ENOTTY;
}

static void fake_rgbled_pwm_init(struct fake_rgbled_pwm_s *pwm)
{
  pwm->ops = &g_fake_rgbled_pwm_ops;
  memset(&pwm->info, 0, sizeof(pwm->info));
  pwm->started = false;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fake_rgbled_initialize
 *
 * Description:
 *   Initialize simulated RGB LED by binding fake PWM lower halves to the
 *   standard NuttX RGB LED upper-half driver.
 *
 ****************************************************************************/

int fake_rgbled_initialize(int devno)
{
  struct fake_rgbled_dev_s *dev;
  char path[32];
  int ret;

  dev = kmm_zalloc(sizeof(struct fake_rgbled_dev_s));
  if (dev == NULL)
    {
      return -ENOMEM;
    }

  fake_rgbled_pwm_init(&dev->red);
  fake_rgbled_pwm_init(&dev->green);
  fake_rgbled_pwm_init(&dev->blue);

  snprintf(path, sizeof(path), "/dev/rgbled%d", devno);

  ret = rgbled_register(path,
                        (struct pwm_lowerhalf_s *)&dev->red,
                        (struct pwm_lowerhalf_s *)&dev->red,
                        (struct pwm_lowerhalf_s *)&dev->red,
                        1,
                        2,
                        3);
  if (ret < 0)
    {
      kmm_free(dev);
    }

  return ret;
}
