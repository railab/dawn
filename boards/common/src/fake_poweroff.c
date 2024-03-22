/*
 * boards/common/src/fake_poweroff.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <nuttx/board.h>

#include <debug.h>

/* No board-specific header needed */

/****************************************************************************
 * Public Functions
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_POWEROFF
/****************************************************************************
 * Name: board_power_off
 *
 * Description:
 *   Power off the board.  This function may or may not be supported by a
 *   particular board architecture.
 *
 * Input Parameters:
 *   status - Status information provided with the power off event.
 *
 * Returned Value:
 *   If this function returns, then it was not possible to power-off the
 *   board due to some constraints.  The return value int this case is a
 *   board-specific reason for the failure to shutdown.
 *
 ****************************************************************************/

int board_power_off(int status)
{
  /* Do nothing here */

  _info("board_power_off %d\n", status);

  return 0;
}
#endif

/****************************************************************************
 * Name: fake_poweroff_initialize
 *
 * Description:
 *   Initialize fake poweroff support (no-op, callback is enough)
 *
 ****************************************************************************/

void fake_poweroff_initialize(void)
{
  /* No initialization needed - board_power_off callback will be called
   * by NuttX boardctl() interface
   */
}
