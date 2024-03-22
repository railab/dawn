// dawn/include/dawn/proto/nimble/prph_dis.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"
#include "dawn/proto/nimble/iprph.hxx"
#include "host/ble_gatt.h"

namespace dawn
{
/**
 * @brief Device Information Service (DIS) for BLE Peripheral.
 *
 * This class implements the Bluetooth SIG Device Information Service (0x180A).
 */

class CProtoNimblePrphDis : public IProtoNimblePrphService
{
public:
  CProtoNimblePrphDis(const SObjectCfg::SObjectCfgItem *item, IProtoNimblePrphCb *cb_);

  ~CProtoNimblePrphDis() override;

  int init() override;
  int deinit() override;
  int start() override;
  int stop() override;

private:
  int id; ///< Service ID assigned by peripheral during registration.

  void configureDesc(const SObjectCfg::SObjectCfgItem *item);
};
} // Namespace dawn
