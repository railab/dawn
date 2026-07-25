// dawn/src/porting/nuttx/board.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <nuttx/board.h>
#include <sys/boardctl.h>

#include "dawn/debug.hxx"
#include "netutils/netinit.h"

#ifdef CONFIG_DAWN_NET_DHCPD
#  include <netinet/in.h>
#  include <unistd.h>
#  include "netutils/dhcpd.h"
#  include "netutils/netlib.h"

// Optional board hook: re-enumerate the USB network gadget after dhcpd is
// running, so a CDC-ECM host (which only DHCPs on link-up) re-runs DHCP with
// the server ready. Weak: boards that do not need it simply omit it.

extern "C" void dawn_board_net_ready(void) __attribute__((weak));
#endif

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

#ifdef CONFIG_DAWN_NET_DHCPD
  /* Start the DHCP server so a USB network gadget (RNDIS) is
   * plug-and-play - the host is auto-configured over the USB link.
   * netinit_bringup() may bring the interface up asynchronously, so wait
   * until it has a valid address before starting the server.
   */

  {
    struct in_addr addr;
    int i;

    for (i = 0; i < 50; i++)
      {
        if (netlib_get_ipv4addr(CONFIG_DAWN_NET_DHCPD_IFNAME, &addr) == OK &&
            addr.s_addr != INADDR_ANY)
          {
            break;
          }

        usleep(100 * 1000);
      }

    ret = dhcpd_start(CONFIG_DAWN_NET_DHCPD_IFNAME);
    if (ret < 0)
      {
        DAWNERR("ERROR: dhcpd_start %d\n", ret);
      }

    /* A CDC-ECM host runs DHCP only once, on the initial USB link-up, which
     * happens at boot before dhcpd is listening - so it fails and falls back
     * to link-local. Ask the board to re-enumerate the USB gadget now that
     * dhcpd is up: the host re-runs DHCP against the ready server and gets a
     * lease, keeping the gadget plug-and-play. (A no-op on boards that do not
     * implement the hook.)
     */

    if (dawn_board_net_ready != nullptr)
      {
        dawn_board_net_ready();
      }
  }
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
