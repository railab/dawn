// dawn/include/dawn/proto/nimble/prph_bas.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/proto/nimble/iprph.hxx"
#include "host/ble_gatt.h"

namespace dawn
{
// Forward reference

class CProtoNimblePrph;

/**
 * @brief Battery Service (BAS) for BLE Peripheral.
 *
 * This class implements the Bluetooth SIG Battery Service (0x180F).
 */

class CProtoNimblePrphBas : public IProtoNimblePrphService
{
public:
  struct
  {
    SObjectId::UObjectId objid; ///< I/O object ID for battery level.
  } typedef SProtoNimblePrphIOBindBas;

  CProtoNimblePrphBas(const SObjectCfg::SObjectCfgItem *item, IProtoNimblePrphCb *cb_);

  ~CProtoNimblePrphBas() override;

  int init() override;
  int deinit() override;
  int start() override;
  int stop() override;

private:
  int id;              ///< Service ID assigned by peripheral during registration.
  SPrphNotiferCb *ncb; ///< Bound battery-level I/O notification context.

  static int notifierCb(void *priv, io_ddata_t *data);

  int bindObject();
  int updateBatteryLevel();
  void configureDesc(const SObjectCfg::SObjectCfgItem *item);
  void allocObject(const SObjectId::UObjectId &obj);
};
} // Namespace dawn
