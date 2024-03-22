/*
 * boards/common/src/fake_dac.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <debug.h>

#include <nuttx/analog/dac.h>

/* No board-specific header needed */

/****************************************************************************
 * Private Types
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fake_dac_dev_s
{
  uint8_t           nochan;
  uint8_t          *chanlist;
  struct dac_dev_s *dev;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* DAC methods */

static void dac_reset(struct dac_dev_s *dev);
static int  dac_setup(struct dac_dev_s *dev);
static void dac_shutdown(struct dac_dev_s *dev);
static void dac_txint(struct dac_dev_s *dev, bool enable);
static int  dac_send(struct dac_dev_s *dev, struct dac_msg_s *msg);
static int  dac_ioctl(struct dac_dev_s *dev, int cmd, unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static const struct dac_ops_s g_dac_ops =
{
  .ao_reset    = dac_reset,
  .ao_setup    = dac_setup,
  .ao_shutdown = dac_shutdown,
  .ao_txint    = dac_txint,
  .ao_send     = dac_send,
  .ao_ioctl    = dac_ioctl,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: dac_reset
 *
 * Description:
 *   Reset the DAC channel.  Called early to initialize the hardware. This
 *   is called, before dac_setup() and on error conditions.
 *
 *   NOTE:  DAC reset will reset both DAC channels!
 *
 * Input Parameters:
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void dac_reset(struct dac_dev_s *dev)
{
}

/****************************************************************************
 * Name: dac_setup
 *
 * Description:
 *   Configure the DAC. This method is called the first time that the DAC
 *   device is opened.  This will occur when the port is first opened.
 *   This setup includes configuring and attaching DAC interrupts.
 *   Interrupts are all disabled upon return.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int dac_setup(struct dac_dev_s *dev)
{
  return OK;
}

/****************************************************************************
 * Name: dac_shutdown
 *
 * Description:
 *   Disable the DAC.  This method is called when the DAC device is closed.
 *   This method reverses the operation the setup method.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void dac_shutdown(struct dac_dev_s *dev)
{
  /* Nothing here */
}

/****************************************************************************
 * Name: dac_txint
 *
 * Description:
 *   Call to enable or disable TX interrupts.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   None
 *
 ****************************************************************************/

static void dac_txint(struct dac_dev_s *dev, bool enable)
{
  /* Nothing here */
}

/****************************************************************************
 * Name: dac_send
 *
 * Description:
 *   Set the DAC output.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int dac_send(struct dac_dev_s *dev, struct dac_msg_s *msg)
{
  struct fake_dac_dev_s *priv = dev->ad_priv;

  UNUSED(priv);

  /* Just print data */

  ainfo("channel=%d, data=%d\n", msg->am_channel, msg->am_data);

  return OK;
}

/****************************************************************************
 * Name: dac_ioctl
 *
 * Description:
 *   All ioctl calls will be routed through this method.
 *
 * Input Parameters:
 *
 * Returned Value:
 *   Zero on success; a negated errno value on failure.
 *
 ****************************************************************************/

static int dac_ioctl(struct dac_dev_s *dev, int cmd, unsigned long arg)
{
  struct fake_dac_dev_s *priv = dev->ad_priv;
  int ret = OK;

  UNUSED(priv);

  switch (cmd)
    {
      default:
        {
          aerr("ERROR: Unknown cmd: %d\n", cmd);
          ret = -ENOTTY;
          break;
        }
    }

  return ret;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fake_dac_initialize
 *
 * Description:
 *   Initialize simulated DAC.
 *
 ****************************************************************************/

int fake_dac_initialize(int devno)
{
  struct fake_dac_dev_s *dac = NULL;
  struct dac_dev_s     *dev = NULL;
  char                  path[32];
  int                   ret;

  /* Alloc memory for private data */

  dac = kmm_zalloc(sizeof(struct fake_dac_dev_s));
  if (!dac)
    {
      aerr("Memory cannot be allocated for DAC\n");
      ret = -ENOMEM;
      goto errout;
    }

  /* Alloc memory for driver data */

  dev = kmm_zalloc(sizeof(struct dac_dev_s));
  if (!dev)
    {
      aerr("Memory cannot be allocated for DAC\n");
      ret = -ENOMEM;
      goto errout;
    }

  /* Initialize upper-half */

  dev->ad_ops  = &g_dac_ops;
  dev->ad_priv = dac;

  /* Get devpath */

  snprintf(path, PATH_MAX, "/dev/dac%d", devno);

  /* Register DAC */

  ret = dac_register(path, dev);
  if (ret < 0)
    {
      aerr("ERROR: dac_register %s failed: %d\n", path, ret);
    }

  return ret;

errout:
  kmm_free(dev);
  kmm_free(dac);

  return ret;
}
