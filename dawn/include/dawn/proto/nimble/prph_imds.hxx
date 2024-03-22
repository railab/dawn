// dawn/include/dawn/proto/nimble/prph_imds.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <map>

#include "dawn/porting/config.hxx"
#include "dawn/proto/nimble/iprph.hxx"
#include "host/ble_gatt.h"

namespace dawn
{
// Forward reference

class CProtoNimblePrph;

/**
 * @brief Industrial Measurement Device Service (IMDS) for BLE Peripheral.
 *
 * This class implements the Bluetooth SIG Industrial Measurement Device
 * Service (0x185A).
 */

class CProtoNimblePrphImds : public IProtoNimblePrphService
{
public:
  constexpr static uint8_t IMDS_CFG0_SIZE_SHIFT = 0;
  constexpr static uint8_t IMDS_CFG0_SIZE_MASK = 0b1111;
  constexpr static uint8_t IMDS_OBJCFG_TYPE_SHIFT = 0;
  constexpr static uint8_t IMDS_OBJCFG_TYPE_MASK = 0b1111;
  constexpr static uint8_t IMDS_EXTCFG_KIND_SHIFT = 0;
  constexpr static uint8_t IMDS_EXTCFG_KIND_MASK = 0xff;
  constexpr static uint8_t IMDS_EXTCFG_SIZE_SHIFT = 8;
  constexpr static uint8_t IMDS_EXTCFG_SIZE_MASK = 0xff;
  constexpr static uint8_t IMDS_DESC_USER_DESCRIPTION = 1 << 0;
  constexpr static uint8_t IMDS_USER_DESCRIPTION_MAX = 16;

  constexpr static uint16_t UUID16_CHR_USER_DESCRIPTION = 0x2901;

  enum
  {
    IMDS_EXT_USER_DESCRIPTION = 1, ///< Characteristic User Description descriptor.
  };

  enum
  {
    PRPH_IMDS_TYPE_TEMP,  ///< Industrial temperature sensor.
    PRPH_IMDS_TYPE_HUM,   ///< Industrial humidity sensor.
    PRPH_IMDS_TYPE_PRESS, ///< Industrial pressure sensor.
    PRPH_IMDS_TYPE_UVIDX, ///< Industrial UV index sensor.
    PRPH_IMDS_TYPE_GAS,   ///< Industrial gas resistance sensor (non-standard
                          ///< UUID).
    PRPH_IMDS_TYPE_LIGHT, ///< Industrial illuminance / light sensor.
  };

  struct
  {
    uint32_t cfg;               ///< Measurement type (ImdsType).
    SObjectId::UObjectId objid; ///< I/O object ID for this measurement.
    uint32_t extCount;          ///< Number of optional descriptor extension records.
  } typedef SProtoNimblePrphIOBindImdsObjid;

  struct
  {
    uint32_t cfg0;   ///< Config 0: number of sensors.
    uint32_t cfg1;   ///< Config 1: reserved.
    uint32_t cfg2;   ///< Config 2: reserved.

    uint32_t data[]; ///< Variable-length measurement bindings and optional descriptor extensions.
  } typedef SProtoNimblePrphIOBindImds;

  CProtoNimblePrphImds(const SObjectCfg::SObjectCfgItem *item, IProtoNimblePrphCb *cb_);

  ~CProtoNimblePrphImds() override;

  int init() override;
  int deinit() override;
  int start() override;
  int stop() override;

  static uint32_t cfgIdIOBindImdsCfgObj(uint8_t type)
  {
    return ((type & IMDS_OBJCFG_TYPE_MASK) << IMDS_OBJCFG_TYPE_SHIFT);
  }

  static uint8_t cfgIdIOBindImdsCfgObjTypeGet(uint32_t val)
  {
    return (val >> IMDS_OBJCFG_TYPE_SHIFT) & IMDS_OBJCFG_TYPE_MASK;
  }

  static uint32_t cfgIdIOBindImdsExt(uint8_t kind, uint8_t size)
  {
    return ((kind & IMDS_EXTCFG_KIND_MASK) << IMDS_EXTCFG_KIND_SHIFT) |
           ((size & IMDS_EXTCFG_SIZE_MASK) << IMDS_EXTCFG_SIZE_SHIFT);
  }

  static uint8_t cfgIdIOBindImdsExtKindGet(uint32_t val)
  {
    return (val >> IMDS_EXTCFG_KIND_SHIFT) & IMDS_EXTCFG_KIND_MASK;
  }

  static uint8_t cfgIdIOBindImdsExtSizeGet(uint32_t val)
  {
    return (val >> IMDS_EXTCFG_SIZE_SHIFT) & IMDS_EXTCFG_SIZE_MASK;
  }

  static uint32_t cfgIdIOBindImdsCfg0(uint8_t size)
  {
    return ((size & IMDS_CFG0_SIZE_MASK) << IMDS_CFG0_SIZE_SHIFT);
  }

  static uint8_t cfgIdIOBindImdsCfg0SizeGet(uint32_t val)
  {
    return (val >> IMDS_CFG0_SIZE_SHIFT) & IMDS_CFG0_SIZE_MASK;
  }

  static uint32_t cfgIdIOBindImdsCfg1(uint8_t res)
  {
    return 0;
  }

  static uint32_t cfgIdIOBindImdsCfg2(uint8_t res)
  {
    return 0;
  }

private:
  int id;                      ///< Service ID assigned by peripheral during registration.
  size_t noChar;               ///< Number of industrial measurement characteristics.
  bool created;                ///< Whether runtime characteristic state has been built.
  struct ble_gatt_svc_def svc; ///< GATT service definition structure.
  std::map<const SObjectId::ObjectId, uint8_t>
    ioTypeMap;                 ///< Map of I/O object ID to measurement type.

  struct SImdsMeta
  {
    uint32_t desc;
    char userDescription[IMDS_USER_DESCRIPTION_MAX];
  };

  struct SImdsDscCb
  {
    uint8_t kind;
    uint8_t len;
    uint8_t data[IMDS_USER_DESCRIPTION_MAX];
  };

  std::map<const SObjectId::ObjectId, SImdsMeta> ioMetaMap; ///< Optional descriptor metadata.

#ifdef CONFIG_DAWN_IO_NOTIFY
  static int notifierCb(void *priv, io_ddata_t *data);
#endif

  template<typename T, size_t WriteBytes = sizeof(T)>
  static int callback(uint16_t conn_handle,
                      uint16_t attr_handle,
                      struct ble_gatt_access_ctxt *ctxt,
                      void *arg);

  static int descriptorCb(uint16_t conn_handle,
                          uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt,
                          void *arg);

  int allocIMDS();
  int createIMDS();
  void deleteIMDS();
  void configureDesc(const SObjectCfg::SObjectCfgItem *item);
  void allocObject(const SProtoNimblePrphIOBindImdsObjid &obj, const uint32_t *ext);
  int configureDescriptors(struct ble_gatt_chr_def *chr, SObjectId::ObjectId objid);
};
} // Namespace dawn
