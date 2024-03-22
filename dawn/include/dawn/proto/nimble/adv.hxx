// dawn/include/dawn/proto/nimble/adv.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"
#include "host/ble_gap.h"

namespace dawn
{
/**
 * @brief BLE advertisement management and GAP interface.
 *
 * This class handles BLE advertisement packet configuration and the GAP
 * (Generic Access Profile) layer.
 */

class CProtoNimbleAdv
{
public:
  static constexpr size_t GAPNAME_MAX =
    CONFIG_DAWN_PROTO_NIMBLE_GAPNAME_MAX; ///< Max GAP device name length (excluding null).
  static_assert(GAPNAME_MAX <= 26, "GAP name must fit in legacy advertising data");
  static uint8_t ownAddrType;             ///< BLE device address type (random/static/public).
  static char gapName[GAPNAME_MAX + 1];   ///< GAP device name visible during BLE discovery.

  CProtoNimbleAdv() = default;
  ~CProtoNimbleAdv() = default;

  static void startAdvertise();
  static void setGapName(const char *name, uint8_t len);

private:
  static void putAd(uint8_t ad_type, uint8_t ad_len, const void *ad, uint8_t *buf, uint8_t *len);

  static void updateAd();
  static int gapEventCb(struct ble_gap_event *event, void *arg);
};

} // Namespace dawn
