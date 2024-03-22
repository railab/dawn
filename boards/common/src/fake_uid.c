/*
 * boards/common/src/fake_uid.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <errno.h>
#include <string.h>

#include <nuttx/board.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#if defined(CONFIG_BOARDCTL_UNIQUEID) && (CONFIG_BOARDCTL_UNIQUEID_SIZE != 16)
#  error "CONFIG_BOARDCTL_UNIQUEID_SIZE must be 16 for simulator"
#endif

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_uniqueid
 *
 * Description:
 *   Get UID
 *
 ****************************************************************************/

int board_uniqueid(uint8_t *uniqueid)
{
  if (uniqueid == 0)
    {
      return -EINVAL;
    }

  memset(uniqueid, 0, CONFIG_BOARDCTL_UNIQUEID_SIZE);

  uniqueid[0] = 0xde;
  uniqueid[1] = 0xad;
  uniqueid[2] = 0xbe;
  uniqueid[3] = 0xef;
  uniqueid[4] = 0xbe;
  uniqueid[5] = 0xef;
  uniqueid[6] = 0xde;
  uniqueid[7] = 0xaf;

  return OK;
}

/****************************************************************************
 * Name: fake_uid_initialize
 *
 * Description:
 *   Initialize fake UID support (no-op, board_uniqueid callback is enough)
 *
 ****************************************************************************/

int fake_uid_initialize(void)
{
  /* No initialization needed - board_uniqueid callback will be called
   * by NuttX boardctl() interface
   */

  return OK;
}
