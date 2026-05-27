/*
 * boards/common/src/fake_pulsecount.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <assert.h>
#include <debug.h>
#include <errno.h>
#include <inttypes.h>
#include <stdio.h>

#include <nuttx/kmalloc.h>
#include <nuttx/timers/pulsecount.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fake_pulsecount_s
{
  const struct pulsecount_ops_s *ops; /* Pulsecount operations */
  int devno;                          /* Device number */
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int sim_pulsecount_setup(struct pulsecount_lowerhalf_s *dev);
static int sim_pulsecount_shutdown(struct pulsecount_lowerhalf_s *dev);
static int sim_pulsecount_start(struct pulsecount_lowerhalf_s *dev,
                                const struct pulsecount_info_s *info,
                                void *handle);
static int sim_pulsecount_stop(struct pulsecount_lowerhalf_s *dev);
static int sim_pulsecount_ioctl(struct pulsecount_lowerhalf_s *dev,
                                int cmd,
                                unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct pulsecount_ops_s g_fake_pulsecount_ops = {
  .setup = sim_pulsecount_setup,
  .shutdown = sim_pulsecount_shutdown,
  .start = sim_pulsecount_start,
  .stop = sim_pulsecount_stop,
  .ioctl = sim_pulsecount_ioctl,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int sim_pulsecount_setup(struct pulsecount_lowerhalf_s *dev)
{
  DEBUGASSERT(dev != NULL);
  return OK;
}

static int sim_pulsecount_shutdown(struct pulsecount_lowerhalf_s *dev)
{
  DEBUGASSERT(dev != NULL);
  return OK;
}

static int sim_pulsecount_start(struct pulsecount_lowerhalf_s *dev,
                                const struct pulsecount_info_s *info,
                                void *handle)
{
  DEBUGASSERT(dev != NULL);
  DEBUGASSERT(info != NULL);

  tmrinfo("START high=%" PRIu32 " low=%" PRIu32 " count=%" PRIu32 "\n",
          info->high_ns, info->low_ns, info->count);

  pulsecount_expired(handle);
  return OK;
}

static int sim_pulsecount_stop(struct pulsecount_lowerhalf_s *dev)
{
  DEBUGASSERT(dev != NULL);
  tmrinfo("STOP\n");
  return OK;
}

static int sim_pulsecount_ioctl(struct pulsecount_lowerhalf_s *dev,
                                int cmd,
                                unsigned long arg)
{
  struct fake_pulsecount_s *priv = (struct fake_pulsecount_s *)dev;

  DEBUGASSERT(dev != NULL);
  UNUSED(priv);
  UNUSED(cmd);
  UNUSED(arg);

  return -ENOTTY;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int fake_pulsecount_initialize(int devno)
{
  struct fake_pulsecount_s *lower = NULL;
  char path[32];

  lower = kmm_zalloc(sizeof(struct fake_pulsecount_s));
  if (lower == NULL)
    {
      tmrerr("ERROR: malloc failed\n");
      return -ENOMEM;
    }

  lower->ops = &g_fake_pulsecount_ops;
  lower->devno = devno;

  snprintf(path, sizeof(path), "/dev/pulsecount%d", devno);

  return pulsecount_register(path, (struct pulsecount_lowerhalf_s *)lower);
}
