// dawn/src/proto/nimble/adv.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/adv.hxx"

#include <cstring>

#include "nimble/nimble_port.h"

using namespace dawn;

uint8_t CProtoNimbleAdv::ownAddrType = 0;
char CProtoNimbleAdv::gapName[CProtoNimbleAdv::GAPNAME_MAX + 1] = "DAWN NimBLE";

static struct ble_npl_callout g_adv_retry_callout;
static bool g_adv_retry_inited = false;

void CProtoNimbleAdv::advRetryCb(struct ble_npl_event *ev)
{
  (void)ev;

  if (ble_gap_adv_active())
    {
      return;
    }

  CProtoNimbleAdv::startAdvertise();
}

void CProtoNimbleAdv::scheduleAdvRetry()
{
  if (!g_adv_retry_inited)
    {
      ble_npl_callout_init(
        &g_adv_retry_callout, nimble_port_get_dflt_eventq(), CProtoNimbleAdv::advRetryCb, nullptr);
      g_adv_retry_inited = true;
    }

  ble_npl_callout_reset(&g_adv_retry_callout, ble_npl_time_ms_to_ticks32(ADV_RETRY_MS));
}

void CProtoNimbleAdv::startAdvertise()
{
  struct ble_gap_adv_params advp;
  int ret;

  printf("advertise\n");

  CProtoNimbleAdv::updateAd();

  std::memset(&advp, 0, sizeof advp);
  advp.conn_mode = BLE_GAP_CONN_MODE_UND;
  advp.disc_mode = BLE_GAP_DISC_MODE_GEN;
  ret = ble_gap_adv_start(CProtoNimbleAdv::ownAddrType,
                          nullptr,
                          BLE_HS_FOREVER,
                          &advp,
                          CProtoNimbleAdv::gapEventCb,
                          nullptr);
  if (ret != 0)
    {
      // A failure here can be transient (e.g. a disconnect racing in-flight
      // notification traffic, with connection resources not yet reclaimed).
      // Giving up would leave the device unreachable until reboot, so keep
      // retrying until advertising is up again.
      DAWNERR(
        "ble_gap_adv_start failed: %d, retry in %u ms\n", ret, static_cast<unsigned>(ADV_RETRY_MS));
      CProtoNimbleAdv::scheduleAdvRetry();
    }
}

void CProtoNimbleAdv::setGapName(const char *name, uint8_t len)
{
  size_t copy;

  if (name == nullptr)
    {
      DAWNERR("NULL GAP name pointer\n");
      return;
    }

  copy = (len > GAPNAME_MAX) ? GAPNAME_MAX : len;
  std::memcpy(CProtoNimbleAdv::gapName, name, copy);
  CProtoNimbleAdv::gapName[copy] = '\0';
}

void CProtoNimbleAdv::putAd(uint8_t type,
                            uint8_t ad_len,
                            const void *ad,
                            uint8_t *buf,
                            uint8_t *len)
{
  buf[(*len)++] = ad_len + 1;
  buf[(*len)++] = type;

  std::memcpy(&buf[*len], ad, ad_len);

  *len += ad_len;
}

void CProtoNimbleAdv::updateAd()
{
  uint8_t ad_flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
  uint8_t ad_len = 0;
  uint8_t ad[BLE_HS_ADV_MAX_SZ];
  size_t gap_len = strnlen(CProtoNimbleAdv::gapName, GAPNAME_MAX);

  CProtoNimbleAdv::putAd(BLE_HS_ADV_TYPE_FLAGS, 1, &ad_flags, ad, &ad_len);
  CProtoNimbleAdv::putAd(BLE_HS_ADV_TYPE_COMP_NAME, gap_len, CProtoNimbleAdv::gapName, ad, &ad_len);

  ble_gap_adv_set_data(ad, ad_len);
}

void CProtoNimbleAdv::requestConnParams(uint16_t conn_handle)
{
  // Opt-in: CONN_ITVL_MIN == 0 leaves the negotiated interval untouched.
  if (CONN_ITVL_MIN == 0)
    {
      return;
    }

  struct ble_gap_upd_params params;
  std::memset(&params, 0, sizeof params);
  params.itvl_min = CONN_ITVL_MIN;
  params.itvl_max = (CONN_ITVL_MAX >= CONN_ITVL_MIN) ? CONN_ITVL_MAX : CONN_ITVL_MIN;
  params.latency = 0;
  params.supervision_timeout = 400; // 4 s (units of 10 ms)

  int ret = ble_gap_update_params(conn_handle, &params);
  if (ret != 0)
    {
      DAWNERR("ble_gap_update_params failed: %d\n", ret);
    }
}

int CProtoNimbleAdv::gapEventCb(struct ble_gap_event *event, void *arg)
{
  switch (event->type)
    {
      case BLE_GAP_EVENT_CONNECT:
        {
          if (event->connect.status)
            {
              CProtoNimbleAdv::startAdvertise();
            }
          else
            {
              CProtoNimbleAdv::requestConnParams(event->connect.conn_handle);
            }
          break;
        }

      case BLE_GAP_EVENT_DISCONNECT:
        {
          DAWNINFO("disconected reason=%d\n", event->disconnect.reason);
          CProtoNimbleAdv::startAdvertise();
          break;
        }
    }

  return 0;
}
