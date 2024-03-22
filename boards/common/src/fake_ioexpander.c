/*
 * boards/common/src/fake_ioexpander.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <errno.h>
#include <debug.h>

#include <nuttx/ioexpander/gpio.h>
#include <nuttx/ioexpander/ioe_dummy.h>

/* No board-specific header needed */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fake_ioexpander_initialize
 *
 * Description:
 *   Initialize simulated GPIO expander.
 *
 ****************************************************************************/

int fake_ioexpander_initialize(void)
{
  /* Get an instance of the simulated I/O expander */

  struct ioexpander_dev_s *ioe = ioe_dummy_initialize();
  if (ioe == NULL)
    {
      gpioerr("ERROR: ioe_dummy_initialize failed\n");
      return -ENOMEM;
    }

  /* Register four pin drivers */

  /* Pin 0: a non-inverted, input pin */

  IOEXP_SETDIRECTION(ioe, 0, IOEXPANDER_DIRECTION_IN);
  IOEXP_SETOPTION(ioe, 0, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 0, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_DISABLE);
  gpio_lower_half(ioe, 0, GPIO_INPUT_PIN, 0);

  /* Pin 1: an inverted, input pin */

  IOEXP_SETDIRECTION(ioe, 1, IOEXPANDER_DIRECTION_IN);
  IOEXP_SETOPTION(ioe, 1, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_INVERT);
  IOEXP_SETOPTION(ioe, 1, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_DISABLE);
  gpio_lower_half(ioe, 1, GPIO_INPUT_PIN, 1);

  /* Pin 2: a non-inverted, input pin */

  IOEXP_SETDIRECTION(ioe, 2, IOEXPANDER_DIRECTION_IN);
  IOEXP_SETOPTION(ioe, 2, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 2, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_DISABLE);
  gpio_lower_half(ioe, 2, GPIO_INPUT_PIN, 2);

  /* Pin 3: a non-inverted, input pin */

  IOEXP_SETDIRECTION(ioe, 3, IOEXPANDER_DIRECTION_IN);
  IOEXP_SETOPTION(ioe, 3, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 3, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_DISABLE);
  gpio_lower_half(ioe, 3, GPIO_INPUT_PIN, 3);

  /* Pin 4: a non-inverted, output pin */

  IOEXP_SETDIRECTION(ioe, 4, IOEXPANDER_DIRECTION_OUT);
  IOEXP_SETOPTION(ioe, 4, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 4, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_DISABLE);
  gpio_lower_half(ioe, 4, GPIO_OUTPUT_PIN, 4);

  /* Pin 5: a non-inverted, output pin */

  IOEXP_SETDIRECTION(ioe, 5, IOEXPANDER_DIRECTION_OUT);
  IOEXP_SETOPTION(ioe, 5, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 5, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_DISABLE);
  gpio_lower_half(ioe, 5, GPIO_OUTPUT_PIN, 5);

  /* Pin 6: a non-inverted, output pin */

  IOEXP_SETDIRECTION(ioe, 6, IOEXPANDER_DIRECTION_OUT);
  IOEXP_SETOPTION(ioe, 6, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 6, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_DISABLE);
  gpio_lower_half(ioe, 6, GPIO_OUTPUT_PIN, 6);

  /* Pin 7: a non-inverted, output pin */

  IOEXP_SETDIRECTION(ioe, 7, IOEXPANDER_DIRECTION_OUT);
  IOEXP_SETOPTION(ioe, 7, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 7, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_DISABLE);
  gpio_lower_half(ioe, 7, GPIO_OUTPUT_PIN, 7);

  /* Pin 8: a non-inverted, edge interrupting pin */

  IOEXP_SETDIRECTION(ioe, 8, IOEXPANDER_DIRECTION_IN);
  IOEXP_SETOPTION(ioe, 8, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 8, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_BOTH);
  gpio_lower_half(ioe, 8, GPIO_INTERRUPT_PIN, 8);

  /* Pin 9: a non-inverted, level interrupting pin */

  IOEXP_SETDIRECTION(ioe, 9, IOEXPANDER_DIRECTION_IN);
  IOEXP_SETOPTION(ioe, 9, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 9, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_HIGH);
  gpio_lower_half(ioe, 9, GPIO_INTERRUPT_PIN, 9);

  /* Pin 10: a non-inverted, level interrupting pin */

  IOEXP_SETDIRECTION(ioe, 10, IOEXPANDER_DIRECTION_IN);
  IOEXP_SETOPTION(ioe, 10, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 10, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_HIGH);
  gpio_lower_half(ioe, 10, GPIO_INTERRUPT_PIN, 10);

  /* Pin 11: a non-inverted, level interrupting pin */

  IOEXP_SETDIRECTION(ioe, 11, IOEXPANDER_DIRECTION_IN);
  IOEXP_SETOPTION(ioe, 11, IOEXPANDER_OPTION_INVERT,
                  (void *)IOEXPANDER_VAL_NORMAL);
  IOEXP_SETOPTION(ioe, 11, IOEXPANDER_OPTION_INTCFG,
                  (void *)IOEXPANDER_VAL_HIGH);
  gpio_lower_half(ioe, 11, GPIO_INTERRUPT_PIN, 11);

  return 0;
}
