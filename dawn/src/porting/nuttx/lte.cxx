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
#include <lte/lte_api.h>
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
// Name: lte_port_set_psave
//***************************************************************************

int lte_port_set_psave(uint8_t mode)
{
  // Apply a power-save mode (PSM / eDRX / none) through the LTE API - the
  // chip-specific AT translation lives in the modem's lapi backend. "none"
  // keeps the device reachable for server-initiated requests; PSM and eDRX
  // trade reachability for power. Can be called at runtime to retune (e.g. a
  // userspace coexistence policy switching modes to let GNSS acquire).

  lte_psm_setting_t psm;
  lte_edrx_setting_t edrx;
  int ret;

  std::memset(&psm, 0, sizeof(psm));
  std::memset(&edrx, 0, sizeof(edrx));

  if (mode == DAWN_LTE_PSAVE_PSM)
    {
      // PSM on, matching Nordic's GNSS sample: long TAU so the device sleeps
      // (freeing the radio for a GNSS cold fix) but a non-zero active time so
      // it keeps a downlink window for server reads after each uplink. The
      // sample uses RAT = 6 s active, RPTAU = several hours.

      psm.enable = LTE_ENABLE;
      psm.req_active_time.unit = LTE_PSM_T3324_UNIT_2SEC;
      psm.req_active_time.time_val = 3; // 3 x 2 s = 6 s active time
      psm.ext_periodic_tau_time.unit = LTE_PSM_T3412_UNIT_10MIN;
      psm.ext_periodic_tau_time.time_val = 6; // 6 x 10 min = 60 min TAU
      edrx.act_type = LTE_EDRX_ACTTYPE_NOTUSE;
      edrx.enable = LTE_DISABLE;
    }
  else if (mode == DAWN_LTE_PSAVE_EDRX)
    {
      // eDRX on (~20 s cycle), PSM off.

      psm.enable = LTE_DISABLE;
      edrx.act_type = LTE_EDRX_ACTTYPE_WBS1;
      edrx.enable = LTE_ENABLE;
      edrx.edrx_cycle = LTE_EDRX_CYC_2048;
      edrx.ptw_val = LTE_EDRX_PTW_512;
    }
  else
    {
      // none: disable both, stay always reachable.

      psm.enable = LTE_DISABLE;
      edrx.act_type = LTE_EDRX_ACTTYPE_NOTUSE;
      edrx.enable = LTE_DISABLE;
    }

  ret = lte_set_psm_sync(&psm);
  if (ret < 0)
    {
      DAWNERR("lte_set_psm_sync failed %d\n", ret);
    }

  ret = lte_set_edrx_sync(&edrx);
  if (ret < 0)
    {
      DAWNERR("lte_set_edrx_sync failed %d\n", ret);
    }

  return OK;
}

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

  // Apply the requested power-save mode. Best effort: handled inside.

  lte_port_set_psave(params->psave_mode);

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
