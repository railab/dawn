// dawn/src/porting/nuttx/board.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <nuttx/board.h>
#include <sys/boardctl.h>

#include "dawn/debug.hxx"
#include "netutils/netinit.h"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: dawn_board_init
//***************************************************************************

int dawn_board_init()
{
  int ret = OK;

#if defined(CONFIG_NET) && !defined(CONFIG_NSH_NETINIT)
  /* Bring up the network */

  netinit_bringup();
#endif

#ifdef CONFIG_BOARDCTL_FINALINIT
  /* Perform architecture-specific final-initialization (if configured) */

  ret = boardctl(BOARDIOC_FINALINIT, 0);
  if (ret < 0)
    {
      DAWNERR("ERROR: BOARDIOC_INIT %d\n", ret);
      return ret;
    }
#endif

  return ret;
}
