/*
 * boards/common/src/fake_encoder.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <errno.h>
#include <stdio.h>

#include <nuttx/kmalloc.h>
#include <nuttx/sensors/qencoder.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fake_encoder_s
{
  FAR const struct qe_ops_s *ops;
  int devno;
  int32_t pos;
  int32_t idx_pos;
  int16_t idx_cnt;
  uint32_t posmax;
  bool reset_on_max;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int fake_encoder_setup(FAR struct qe_lowerhalf_s *lower);
static int fake_encoder_shutdown(FAR struct qe_lowerhalf_s *lower);
static int fake_encoder_position(FAR struct qe_lowerhalf_s *lower,
                                 FAR int32_t *pos);
static int fake_encoder_setposmax(FAR struct qe_lowerhalf_s *lower,
                                  uint32_t pos);
static int fake_encoder_reset(FAR struct qe_lowerhalf_s *lower);
static int fake_encoder_setindex(FAR struct qe_lowerhalf_s *lower,
                                 uint32_t pos);
static int fake_encoder_ioctl(FAR struct qe_lowerhalf_s *lower,
                              int cmd, unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct qe_ops_s g_fake_encoder_ops =
{
  .setup = fake_encoder_setup,
  .shutdown = fake_encoder_shutdown,
  .position = fake_encoder_position,
  .setposmax = fake_encoder_setposmax,
  .reset = fake_encoder_reset,
  .setindex = fake_encoder_setindex,
  .ioctl = fake_encoder_ioctl,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static int fake_encoder_setup(FAR struct qe_lowerhalf_s *lower)
{
  UNUSED(lower);
  return OK;
}

static int fake_encoder_shutdown(FAR struct qe_lowerhalf_s *lower)
{
  UNUSED(lower);
  return OK;
}

static int fake_encoder_position(FAR struct qe_lowerhalf_s *lower,
                                 FAR int32_t *pos)
{
  FAR struct fake_encoder_s *priv = (FAR struct fake_encoder_s *)lower;
  int32_t step = priv->devno + 1;

  priv->pos += step;

  if (priv->posmax > 0 && (uint32_t)priv->pos >= priv->posmax)
    {
      priv->idx_pos = priv->pos;
      priv->idx_cnt++;
      if (priv->reset_on_max)
        {
          priv->pos = 0;
        }
    }

  *pos = priv->pos;
  return OK;
}

static int fake_encoder_setposmax(FAR struct qe_lowerhalf_s *lower,
                                  uint32_t pos)
{
  FAR struct fake_encoder_s *priv = (FAR struct fake_encoder_s *)lower;
  priv->posmax = pos;
  return OK;
}

static int fake_encoder_reset(FAR struct qe_lowerhalf_s *lower)
{
  FAR struct fake_encoder_s *priv = (FAR struct fake_encoder_s *)lower;
  priv->pos = 0;
  priv->idx_pos = 0;
  priv->idx_cnt = 0;
  return OK;
}

static int fake_encoder_setindex(FAR struct qe_lowerhalf_s *lower,
                                 uint32_t pos)
{
  FAR struct fake_encoder_s *priv = (FAR struct fake_encoder_s *)lower;
  priv->idx_pos = (int32_t)pos;
  priv->idx_cnt++;
  return OK;
}

static int fake_encoder_ioctl(FAR struct qe_lowerhalf_s *lower,
                              int cmd, unsigned long arg)
{
  FAR struct fake_encoder_s *priv = (FAR struct fake_encoder_s *)lower;

  if (cmd == QEIOC_GETINDEX)
    {
      FAR struct qe_index_s *idx = (FAR struct qe_index_s *)(uintptr_t)arg;
      if (idx == NULL)
        {
          return -EINVAL;
        }

      idx->qenc_pos = priv->pos;
      idx->indx_pos = priv->idx_pos;
      idx->indx_cnt = priv->idx_cnt;
      return OK;
    }

  return -ENOTTY;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

int fake_encoder_initialize(int devno)
{
  FAR struct fake_encoder_s *priv;
  char path[32];

  priv = kmm_zalloc(sizeof(struct fake_encoder_s));
  if (priv == NULL)
    {
      return -ENOMEM;
    }

  priv->ops          = &g_fake_encoder_ops;
  priv->devno        = devno;
  priv->pos          = 0;
  priv->idx_pos      = 0;
  priv->idx_cnt      = 0;
  priv->posmax       = 0;
  priv->reset_on_max = true;

  snprintf(path, sizeof(path), "/dev/qe%d", devno);
  return qe_register(path, (FAR struct qe_lowerhalf_s *)priv);
}
