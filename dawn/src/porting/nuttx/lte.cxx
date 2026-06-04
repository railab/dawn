// dawn/src/porting/nuttx/lte.cxx
//
// SPDX-License-Identifier: Apache-2.0
//
// LTE data-connection bring-up implemented on top of the common (target
// independent) NuttX LTE API (lapi).  This works with any modem that
// provides an LTE API backend - there must be NO chip-specific dependency
// here.  If a target's modem lacks a lapi backend, that backend must be
// added in nuttx (arch) / nuttx-apps, not worked around here.

#include <cerrno>
#include <cstring>

#include <lte/lapi.h>
#include <nuttx/wireless/lte/lte.h>

#include "dawn/debug.hxx"
#include "dawn/porting/lte.hxx"

using namespace dawn;

//***************************************************************************
// Private Data
//***************************************************************************

static uint32_t g_lte_status = DAWN_LTE_STATUS_DOWN;

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: lte_port_connect
//***************************************************************************

int lte_port_connect(const struct dawn::SLteParams *params)
{
  lte_apn_setting_t apn;
  lte_pdn_t pdn;
  int ret;

  // Initialize the LTE API.  -EALREADY just means it is already up.

  ret = lte_initialize();
  if (ret < 0 && ret != -EALREADY)
    {
      DAWNERR("lte_initialize failed %d\n", ret);
      return ret;
    }

  // Power on the modem.

  ret = lte_power_on();
  if (ret < 0 && ret != -EALREADY)
    {
      DAWNERR("lte_power_on failed %d\n", ret);
      return ret;
    }

  // Turn the radio on and search/attach to the network (blocking).

  ret = lte_radio_on_sync();
  if (ret < 0)
    {
      DAWNERR("lte_radio_on_sync failed %d\n", ret);
      return ret;
    }

  // Activate the packet data network (assigns an IP address).  An APN is
  // generally required; when none is configured we still pass an empty APN
  // with the initial-attach type to let the network/SIM pick the default.

  std::memset(&apn, 0, sizeof(apn));

  apn.apn = const_cast<char *>(params->apn != nullptr ? params->apn : "");
  apn.ip_type = params->ip_type;
  apn.auth_type = params->auth_type;
  apn.apn_type = LTE_APN_TYPE_DEFAULT | LTE_APN_TYPE_IA;

  if (params->username != nullptr && params->username[0] != '\0')
    {
      apn.user_name = const_cast<char *>(params->username);
    }

  if (params->password != nullptr && params->password[0] != '\0')
    {
      apn.password = const_cast<char *>(params->password);
    }

  ret = lte_activate_pdn_sync(&apn, &pdn);
  if (ret < 0)
    {
      DAWNERR("lte_activate_pdn_sync failed %d (apn='%s')\n", ret, apn.apn);
      return ret;
    }

  DAWNINFO("LTE connected: session %d, %d address(es)\n", pdn.session_id, pdn.ipaddr_num);

  g_lte_status = DAWN_LTE_STATUS_CONNECTED;

  return OK;
}

//***************************************************************************
// Name: lte_port_disconnect
//***************************************************************************

int lte_port_disconnect(void)
{
  int ret;

  ret = lte_power_off();
  if (ret < 0 && ret != -EALREADY)
    {
      DAWNERR("lte_power_off failed %d\n", ret);
    }

  g_lte_status = DAWN_LTE_STATUS_DOWN;

  return OK;
}

//***************************************************************************
// Name: lte_port_status
//***************************************************************************

int lte_port_status(uint32_t *status)
{
  *status = g_lte_status;
  return OK;
}
