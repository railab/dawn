/*
 * boards/common/src/fake_userleds.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <nuttx/kmalloc.h>

#include <errno.h>
#include <stdio.h>

#include <nuttx/leds/userled.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct userled_sim_s
{
  struct userled_lowerhalf_s lower;
  uint8_t                    lednum;
  uint32_t                   ledstate;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static userled_set_t
userled_supported(const struct userled_lowerhalf_s *lower);
static void userled_setled(const struct userled_lowerhalf_s *lower,
                           int led, bool ledon);
static void userled_setall(const struct userled_lowerhalf_s *lower,
                           userled_set_t ledset);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: userled_supported
 *
 * Description:
 *   Return the set of LEDs supported by the board
 *
 ****************************************************************************/

static userled_set_t
userled_supported(const struct userled_lowerhalf_s *lower)
{
  struct userled_sim_s *priv = (struct userled_sim_s *)lower;

  return (userled_set_t)((1 << priv->lednum) - 1);
}

/****************************************************************************
 * Name: userled_setled
 *
 * Description:
 *   Set the current state of one LED
 *
 ****************************************************************************/

static void userled_setled(const struct userled_lowerhalf_s *lower,
                           int led, bool ledon)
{
  struct userled_sim_s *priv = (struct userled_sim_s *)lower;

  if (ledon)
    {
      priv->ledstate |= (led << 1);
    }
  else
    {
      priv->ledstate &= ~(led << 1);
    }
}

/****************************************************************************
 * Name: userled_setall
 *
 * Description:
 *   Set the state of all LEDs
 *
 ****************************************************************************/

static void userled_setall(const struct userled_lowerhalf_s *lower,
                           userled_set_t ledset)
{
  struct userled_sim_s *priv = (struct userled_sim_s *)lower;

  priv->ledstate = ledset & ((1 << priv->lednum) - 1);
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fake_userleds_initialize
 *
 * Description:
 *   Initialize simulated LEDS.
 *
 ****************************************************************************/

int fake_userleds_initialize(int devno, uint8_t lednum)
{
  struct userled_sim_s *priv = kmm_zalloc(sizeof(struct userled_sim_s));
  char                  path[32];

  if (!priv)
    {
      return -ENOMEM;
    }

  priv->lednum   = lednum;
  priv->ledstate = 0;

  /* Get ops */

  priv->lower.ll_supported = userled_supported;
  priv->lower.ll_setled    = userled_setled;
  priv->lower.ll_setall    = userled_setall;

  snprintf(path, PATH_MAX, "/dev/leds%d", devno);

  return userled_register(path, &priv->lower);
}
