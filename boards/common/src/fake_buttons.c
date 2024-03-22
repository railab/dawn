/*
 * boards/common/src/fake_buttons.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <nuttx/kmalloc.h>

#include <errno.h>
#include <stdio.h>

#include <nuttx/input/buttons.h>

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct btn_sim_s
{
  struct btn_lowerhalf_s lower;
  uint8_t                btnnum;
  uint8_t                devno;
  uint32_t               btnstate;
};

/****************************************************************************
 * Private Data
 ****************************************************************************/

static uint8_t g_btn_devno = 0;

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static btn_buttonset_t
btn_supported(const struct btn_lowerhalf_s *lower);
static btn_buttonset_t btn_buttons(const struct btn_lowerhalf_s *lower);
static void btn_enable(const struct btn_lowerhalf_s *lower,
                       btn_buttonset_t press, btn_buttonset_t release,
                       btn_handler_t handler, void *arg);

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: btn_supported
 *
 * Description:
 *   Return the set of buttons supported
 *
 ****************************************************************************/

static btn_buttonset_t btn_supported(const struct btn_lowerhalf_s *lower)
{
  struct btn_sim_s *priv = (struct btn_sim_s *)lower;

  return (btn_buttonset_t)((1 << priv->btnnum) - 1);
}

/****************************************************************************
 * Name: btn_buttons
 *
 * Description:
 *   Return the current state of button data
 *
 ****************************************************************************/

static btn_buttonset_t btn_buttons(const struct btn_lowerhalf_s *lower)
{
  struct btn_sim_s *priv = (struct btn_sim_s *)lower;
  uint32_t retval = priv->btnstate;

  if (priv->btnstate & (1 << priv->devno))
    {
      priv->btnstate = 0;
    }
  else
    {
      priv->btnstate = 1 << priv->devno;
    }

  return retval;
}

/****************************************************************************
 * Name: btn_enable
 *
 * Description:
 *   Enable interrupts on the selected set of buttons.  And empty set or
 *   a NULL handler will disable all interrupts.
 *
 ****************************************************************************/

static void btn_enable(const struct btn_lowerhalf_s *lower,
                       btn_buttonset_t press, btn_buttonset_t release,
                       btn_handler_t handler, void *arg)
{
  /* Empty */
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fake_buttons_initialize
 *
 * Description:
 *   Initialize simulated buttons.
 *
 ****************************************************************************/

int fake_buttons_initialize(int devno, uint8_t btnnum)
{
  struct btn_sim_s *priv = kmm_zalloc(sizeof(struct btn_sim_s));
  char              path[32];

  if (!priv)
    {
      return -ENOMEM;
    }

  priv->btnnum   = btnnum;
  priv->btnstate = 1;
  priv->devno    = g_btn_devno++;

  /* Get ops */

  priv->lower.bl_supported = btn_supported;
  priv->lower.bl_buttons   = btn_buttons;
  priv->lower.bl_enable    = btn_enable;

  snprintf(path, PATH_MAX, "/dev/buttons%d", devno);

  return btn_register(path, &priv->lower);
}
