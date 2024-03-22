/*
 * boards/common/src/fake_adc.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <stdio.h>
#include <debug.h>
#include <assert.h>

#include <nuttx/kmalloc.h>
#include <nuttx/kthread.h>
#include <nuttx/signal.h>

#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>

#ifndef ANIOC_STOP
#define ANIOC_STOP 0x1001
#endif

#ifndef ANIOC_SET_TIMER_FREQ
#define ANIOC_SET_TIMER_FREQ 0x1002
#endif

#ifndef ANIOC_SAMPLES_ON_READ
#define ANIOC_SAMPLES_ON_READ 0x1003
#endif

/* No board-specific header needed */

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fake_adc_dev_s
{
  uint8_t                      nochan;
  uint8_t                     *chanlist;
  uint16_t                    *buffer;
  uint32_t                     interval;
  volatile bool                rxen;
  volatile bool                trigger;
  volatile uint32_t            pending_samples;
  const struct adc_callback_s *cb;
  struct adc_dev_s            *dev;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

/* ADC Driver Methods */

static int  adc_bind(struct adc_dev_s *dev,
                     const struct adc_callback_s *callback);
static void adc_reset(struct adc_dev_s *dev);
static int  adc_setup(struct adc_dev_s *dev);
static void adc_shutdown(struct adc_dev_s *dev);
static void adc_rxint(struct adc_dev_s *dev, bool enable);
static int  adc_ioctl(struct adc_dev_s *dev, int cmd, unsigned long arg);

/****************************************************************************
 * Private Data
 ****************************************************************************/

/* ADC interface operations */

static const struct adc_ops_s g_adc_ops =
{
  .ao_bind     = adc_bind,
  .ao_reset    = adc_reset,
  .ao_setup    = adc_setup,
  .ao_shutdown = adc_shutdown,
  .ao_rxint    = adc_rxint,
  .ao_ioctl    = adc_ioctl,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: adc_bind
 *
 * Description:
 *   Bind the upper-half driver callbacks to the lower-half implementation.
 *   This must be called early in order to receive ADC event notifications.
 *
 ****************************************************************************/

static int adc_bind(struct adc_dev_s *dev,
                    const struct adc_callback_s *callback)
{
  struct fake_adc_dev_s *priv = (struct fake_adc_dev_s *)dev->ad_priv;

  DEBUGASSERT(priv != NULL);
  priv->cb = callback;

  return OK;
}

/****************************************************************************
 * Name: adc_reset
 *
 * Description:
 *   Reset the ADC device.  Called early to initialize the hardware.
 *   This is called, before adc_setup() and on error conditions.
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

static void adc_reset(struct adc_dev_s *dev)
{
  /* empty for now */
}

/****************************************************************************
 * Name: adc_setup
 *
 * Description:
 *   Configure the ADC. This method is called the first time that the ADC
 *   device is opened.  This will occur when the port is first opened.
 *   This setup includes configuring and attaching ADC interrupts.
 *   Interrupts are all disabled upon return.
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

static int adc_setup(struct adc_dev_s *dev)
{
  /* empty for now */

  return OK;
}

/****************************************************************************
 * Name: adc_shutdown
 *
 * Description:
 *   Disable the ADC.  This method is called when the ADC device is closed.
 *   This method reverses the operation the setup method.
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

static void adc_shutdown(struct adc_dev_s *dev)
{
  struct fake_adc_dev_s *priv = (struct fake_adc_dev_s *)dev->ad_priv;

  priv->rxen            = false;
  priv->trigger         = false;
  priv->pending_samples = 0;
}

/****************************************************************************
 * Name: adc_rxint
 *
 * Description:
 *   Call to enable or disable RX interrupts.
 *
 * Input Parameters:
 *
 * Returned Value:
 *
 ****************************************************************************/

static void adc_rxint(struct adc_dev_s *dev, bool enable)
{
  struct fake_adc_dev_s *priv = (struct fake_adc_dev_s *)dev->ad_priv;

  priv->rxen = enable;
}

/****************************************************************************
 * Name: adc_ioctl
 *
 * Description:
 *   All ioctl calls will be routed through this method.
 *
 * Input Parameters:
 *   dev - pointer to device structure used by the driver
 *   cmd - command
 *   arg - arguments passed with command
 *
 * Returned Value:
 *
 ****************************************************************************/

static int adc_ioctl(struct adc_dev_s *dev, int cmd, unsigned long arg)
{
  struct fake_adc_dev_s *priv = (struct fake_adc_dev_s *)dev->ad_priv;
  int ret = -ENOTTY;

  switch (cmd)
    {
      case ANIOC_TRIGGER:
        {
          priv->trigger = true;
          ret = OK;
          break;
        }

      case ANIOC_STOP:
        {
          priv->rxen            = false;
          priv->trigger         = false;
          priv->pending_samples = 0;
          ret                   = OK;
          break;
        }

      case ANIOC_SET_TIMER_FREQ:
        {
          if (arg == 0)
            {
              ret = -EINVAL;
            }
          else
            {
              priv->interval = 1000000ul / arg;
              if (priv->interval == 0)
                {
                  priv->interval = 1;
                }

              ret = OK;
            }

          break;
        }

      case ANIOC_SAMPLES_ON_READ:
        {
          ret = priv->pending_samples;
          break;
        }

      case ANIOC_GET_NCHANNELS:
        {
          ret = priv->nochan;
          break;
        }
    }

  return ret;
}

/****************************************************************************
 * Name: fakeadc_thread
 *
 * Description:
 *   Samples thread.
 *
 ****************************************************************************/

static int fakeadc_thread(int argc, char **argv)
{
  struct fake_adc_dev_s *priv = (FAR struct fake_adc_dev_s *)
    ((uintptr_t)strtoul(argv[1], NULL, 16));
  struct adc_dev_s *dev      = (struct adc_dev_s *)priv->dev;
  int i;

  while (true)
    {
      /* Sleeping thread for interval */

      nxsig_usleep(priv->interval);

      /* Feed data */

      if ((priv->trigger || priv->rxen) && priv->cb != NULL)
        {
          for (i = 0; i < priv->nochan; i++)
            {
              priv->cb->au_receive(dev, priv->chanlist[i], priv->buffer[i]);
            }

          priv->pending_samples += priv->nochan;
          priv->trigger = false;
        }
    }

  return 0;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fake_adc_initialize
 *
 * Description:
 *   Initialize simulated ADC.
 *
 ****************************************************************************/

int fake_adc_initialize(int devno)
{
  struct fake_adc_dev_s *adc = NULL;
  struct adc_dev_s     *dev = NULL;
  char                  path[32];
  char                 *argv[2];
  char                  arg1[32];
  int                   ret;
  int                   i;

  /* Alloc memory for private data */

  adc = kmm_zalloc(sizeof(struct fake_adc_dev_s));
  if (!adc)
    {
      aerr("Memory cannot be allocated for ADC\n");
      ret = -ENOMEM;
      goto errout;
    }

  /* Alloc memory for driver data */

  dev = kmm_zalloc(sizeof(struct adc_dev_s));
  if (!dev)
    {
      aerr("Memory cannot be allocated for ADC\n");
      ret = -ENOMEM;
      goto errout;
    }

  /* Intiialzie lower-half */

  adc->interval = 20000;
  adc->nochan   = 32;
  adc->dev      = dev;

  /* Allocate channel list */

  adc->chanlist = kmm_zalloc(adc->nochan * sizeof(uint8_t));
  if (!adc->chanlist)
    {
      aerr("Memory cannot be allocated for ADC\n");
      ret = -ENOMEM;
      goto errout;
    }

  /* Allocate buffer for samples*/

  adc->buffer = kmm_zalloc(adc->nochan * sizeof(uint16_t));
  if (!adc->buffer)
    {
      aerr("Memory cannot be allocated for ADC\n");
      ret = -ENOMEM;
      goto errout;
    }

  /* Initialize channel list and buffer data*/

  for (i = 0; i < adc->nochan; i++)
    {
      adc->chanlist[i] = i;
      adc->buffer[i]   = i + devno;
    }

  /* Initialize upper-half */

  dev->ad_ops  = &g_adc_ops;
  dev->ad_priv = adc;

  /* Create thread for sensor */

  snprintf(arg1, 32, "%p", adc);
  argv[0] = arg1;
  argv[1] = NULL;
  ret = kthread_create("adc_thread", SCHED_PRIORITY_DEFAULT,
                       CONFIG_DEFAULT_TASK_STACKSIZE,
                       fakeadc_thread, argv);
  if (ret < 0)
    {
      ret = errno;
      goto errout;
    }

  /* Get path */

  snprintf(path, PATH_MAX, "/dev/adc%d", devno);

  /* Register ADC */

  ret = adc_register(path, dev);
  if (ret < 0)
    {
      aerr("ERROR: adc_register %s failed: %d\n", path, ret);
    }

  return ret;

errout:
  kmm_free(dev);
  kmm_free(adc);

  return ret;
}
