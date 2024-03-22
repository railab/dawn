// dawn/include/dawn/proto/nimble/prph_custom.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/proto/nimble/iprph.hxx"
#include "host/ble_gatt.h"

namespace dawn
{
/**
 * @brief Custom Service defined by user.
 *
 * This class implements custom services.
 */

class CProtoNimblePrphCustom : public IProtoNimblePrphService
{
public:
  enum
  {
    CUSTOM_CHR_F_READ = 0x01,
    CUSTOM_CHR_F_WRITE = 0x02,
    CUSTOM_CHR_F_NOTIFY = 0x04,
  };

  struct
  {
    uint32_t flags;
    uint32_t uuid[4];
    SObjectId::UObjectId objid;
  } typedef SProtoNimblePrphCustomChar;

  struct
  {
    uint32_t cfg0;
    uint32_t cfg1;
    uint32_t cfg2;
    uint32_t uuid[4];
    SProtoNimblePrphCustomChar chr[];
  } typedef SProtoNimblePrphCustomSvc;

  struct SCustomCharCb
  {
    CIOCommon *io;
    uint16_t handle;
    io_ddata_t *data;
  };

  struct SCustomChar
  {
    SObjectId::ObjectId objid;
    uint8_t flags;
    uint8_t uuid[16];
  };

  explicit CProtoNimblePrphCustom(const SObjectCfg::SObjectCfgItem *item, IProtoNimblePrphCb *cb_);
  ~CProtoNimblePrphCustom() override;

  int init() override;
  int deinit() override;
  int start() override;
  int stop() override;

private:
  int id;
  bool created;
  ble_gatt_svc_def svc;
  ble_gatt_chr_def *chrs;
  std::vector<SCustomChar> vchars;
  std::vector<SCustomCharCb *> vcbs;

#ifdef CONFIG_DAWN_IO_NOTIFY
  static int notifierCb(void *priv, io_ddata_t *data);
#endif
  static int callback(uint16_t conn_handle,
                      uint16_t attr_handle,
                      struct ble_gatt_access_ctxt *ctxt,
                      void *arg);

  int allocCustom();
  int createCustom();
  void deleteCustom();
  void configureDesc(const SObjectCfg::SObjectCfgItem *item);
};

} // namespace dawn
