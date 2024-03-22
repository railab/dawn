/*
 * boards/common/src/fake_reset.c
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

#ifdef CONFIG_BOARDCTL_RESET
/****************************************************************************
 * Name: board_reset
 *
 * Description:
 *   Reset board.  Support for this function is required by board-level
 *   logic if CONFIG_BOARDCTL_RESET is selected.
 *
 * Input Parameters:
 *   status - Status information provided with the reset event.  This
 *            meaning of this status information is board-specific.  If not
 *            used by a board, the value zero may be provided in calls to
 *            board_reset().
 *
 * Returned Value:
 *   If this function returns, then it was not possible to power-off the
 *   board due to some constraints.  The return value int this case is a
 *   board-specific reason for the failure to shutdown.
 *
 ****************************************************************************/

int board_reset(int status)
{
  /* Do nothing here */

  _info("board_reset %d\n", status);

  return 0;
}
#endif

/****************************************************************************
 * Name: board_reset_cause
 *
 * Description:
 *   Get the cause of last board reset. This should call architecture
 *   specific logic to handle the register read.
 *
 * Input Parameters:
 *   cause - Pointer to boardioc_reset_cause_s structure to which the
 *      reason (and potentially subreason) is saved.
 *
 * Returned Value:
 *   This functions should always return succesfully with 0. We save
 *   BOARDIOC_RESETCAUSE_UNKOWN in cause structure if we are
 *   not able to get last reset cause from HW (which is unlikely).
 *
 ****************************************************************************/

#ifdef CONFIG_BOARDCTL_RESET_CAUSE
int board_reset_cause(struct boardioc_reset_cause_s *cause)
{
  /* Dummy data */

  cause->cause = 0xdeadbeef;
  cause->flag  = 0xbeefdead;

  return 0;
}
#endif /* CONFIG_BOARDCTL_RESET_CAUSE */

/****************************************************************************
 * Name: fake_reset_initialize
 *
 * Description:
 *   Initialize fake reset support (no-op, callbacks are enough)
 *
 ****************************************************************************/

void fake_reset_initialize(void)
{
  /* No initialization needed - board_reset/board_reset_cause callbacks
   * will be called by NuttX boardctl() interface
   */
}
