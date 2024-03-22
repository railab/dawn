// dawn/src/proto/nimble/adv.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/adv.hxx"

#include <cstring>

using namespace dawn;

uint8_t CProtoNimbleAdv::ownAddrType = 0;
char CProtoNimbleAdv::gapName[CProtoNimbleAdv::GAPNAME_MAX + 1] = "DAWN NimBLE";

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
      DAWNERR("ble_gap_adv_start failed: %d\n", ret);
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
