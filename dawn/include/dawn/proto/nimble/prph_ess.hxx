// dawn/include/dawn/proto/nimble/prph_ess.hxx
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
 * @brief Environmental Sensing Service (ESS) for BLE Peripheral.
 *
 * This class implements the Bluetooth SIG Environmental Sensing Service
 * (0x181A).
 */

class CProtoNimblePrphEss : public IProtoNimblePrphService
{
public:
  constexpr static uint8_t ESS_CFG0_SIZE_SHIFT = 0;
  constexpr static uint8_t ESS_CFG0_SIZE_MASK = 0b1111;
  constexpr static uint8_t ESS_OBJCFG_TYPE_SHIFT = 0;
  constexpr static uint8_t ESS_OBJCFG_TYPE_MASK = 0b1111;
  constexpr static uint8_t ESS_EXTCFG_KIND_SHIFT = 0;
  constexpr static uint8_t ESS_EXTCFG_KIND_MASK = 0xff;
  constexpr static uint8_t ESS_EXTCFG_SIZE_SHIFT = 8;
  constexpr static uint8_t ESS_EXTCFG_SIZE_MASK = 0xff;
  constexpr static uint8_t ESS_DESC_USER_DESCRIPTION = 1 << 0;
  constexpr static uint8_t ESS_DESC_VALID_RANGE = 1 << 1;
  constexpr static uint8_t ESS_DESC_MEASUREMENT = 1 << 2;
  constexpr static uint8_t ESS_DESC_CONFIGURATION = 1 << 3;
  constexpr static uint8_t ESS_DESC_TRIGGER_SETTING = 1 << 4;
  constexpr static uint8_t ESS_USER_DESCRIPTION_MAX = 16;
  constexpr static uint8_t ESS_RAW_DESCRIPTOR_MAX = 16;

  constexpr static uint16_t UUID16_CHR_USER_DESCRIPTION = 0x2901;
  constexpr static uint16_t UUID16_VALID_RANGE = 0x2906;
  constexpr static uint16_t UUID16_ES_CONFIGURATION = 0x290b;
  constexpr static uint16_t UUID16_ES_MEASUREMENT = 0x290c;
  constexpr static uint16_t UUID16_ES_TRIGGER_SETTING = 0x290d;

  enum
  {
    ESS_EXT_USER_DESCRIPTION = 1, ///< Characteristic User Description descriptor.
    ESS_EXT_VALID_RANGE,          ///< Valid Range descriptor.
    ESS_EXT_MEASUREMENT,          ///< Environmental Sensing Measurement descriptor.
    ESS_EXT_CONFIGURATION,        ///< Environmental Sensing Configuration descriptor.
    ESS_EXT_TRIGGER_SETTING,      ///< Environmental Sensing Trigger Setting descriptor.
  };

  enum
  {
    PRPH_ESS_TYPE_TEMP,       ///< Temperature sensor.
    PRPH_ESS_TYPE_HUM,        ///< Humidity sensor.
    PRPH_ESS_TYPE_PRESS,      ///< Pressure (barometric) sensor.
    PRPH_ESS_TYPE_UVIDX,      ///< UV index sensor.
    PRPH_ESS_TYPE_TWINDSPEED, ///< Wind speed sensor.
    PRPH_ESS_TYPE_TWINDDIR,   ///< Wind direction sensor.
    PRPH_ESS_TYPE_GAS,        ///< Gas resistance sensor (non-standard UUID).
    PRPH_ESS_TYPE_LIGHT,      ///< Illuminance / light sensor.
  };

  struct
  {
    uint32_t cfg;               ///< Sensor type (EssType).
    SObjectId::UObjectId objid; ///< I/O object ID for this sensor data.
    uint32_t extCount;          ///< Number of optional descriptor extension records.
  } typedef SProtoNimblePrphIOBindEssObjid;

  struct
  {
    uint32_t cfg0;   ///< Config 0: number of sensors.
    uint32_t cfg1;   ///< Config 1: reserved.
    uint32_t cfg2;   ///< Config 2: reserved.

    uint32_t data[]; ///< Variable-length sensor bindings and optional descriptor extensions.
  } typedef SProtoNimblePrphIOBindEss;

  CProtoNimblePrphEss(const SObjectCfg::SObjectCfgItem *item, IProtoNimblePrphCb *cb_);

  ~CProtoNimblePrphEss() override;

  int init() override;
  int deinit() override;
  int start() override;
  int stop() override;

  static uint32_t cfgIdIOBindEssCfgObj(uint8_t type)
  {
    return ((type & ESS_OBJCFG_TYPE_MASK) << ESS_OBJCFG_TYPE_SHIFT);
  }

  static uint8_t cfgIdIOBindEssCfgObjTypeGet(uint32_t val)
  {
    return (val >> ESS_OBJCFG_TYPE_SHIFT) & ESS_OBJCFG_TYPE_MASK;
  }

  static uint32_t cfgIdIOBindEssExt(uint8_t kind, uint8_t size)
  {
    return ((kind & ESS_EXTCFG_KIND_MASK) << ESS_EXTCFG_KIND_SHIFT) |
           ((size & ESS_EXTCFG_SIZE_MASK) << ESS_EXTCFG_SIZE_SHIFT);
  }

  static uint8_t cfgIdIOBindEssExtKindGet(uint32_t val)
  {
    return (val >> ESS_EXTCFG_KIND_SHIFT) & ESS_EXTCFG_KIND_MASK;
  }

  static uint8_t cfgIdIOBindEssExtSizeGet(uint32_t val)
  {
    return (val >> ESS_EXTCFG_SIZE_SHIFT) & ESS_EXTCFG_SIZE_MASK;
  }

  static uint32_t cfgIdIOBindEssCfg0(uint8_t size)
  {
    return ((size & ESS_CFG0_SIZE_MASK) << ESS_CFG0_SIZE_SHIFT);
  }

  static uint8_t cfgIdIOBindEssCfg0SizeGet(uint32_t val)
  {
    return (val >> ESS_CFG0_SIZE_SHIFT) & ESS_CFG0_SIZE_MASK;
  }

  static uint32_t cfgIdIOBindEssCfg1(uint8_t res)
  {
    return 0;
  }

  static uint32_t cfgIdIOBindEssCfg2(uint8_t res)
  {
    return 0;
  }

private:
  int id;                      ///< Service ID assigned by peripheral during registration.
  size_t noChar;               ///< Number of environmental characteristics in this service.
  bool created;                ///< Whether runtime characteristic state has been built.
  struct ble_gatt_svc_def svc; ///< GATT service definition structure.
  std::map<const SObjectId::ObjectId, uint8_t> ioTypeMap; ///< Map of I/O object ID to sensor type.

  struct SEssMeta
  {
    uint32_t desc;
    uint32_t validMin;
    uint32_t validMax;
    SObjectId::ObjectId measurementFlagsObjid;
    SObjectId::ObjectId samplingFunctionObjid;
    SObjectId::ObjectId measurementPeriodObjid;
    SObjectId::ObjectId updateIntervalObjid;
    SObjectId::ObjectId applicationObjid;
    SObjectId::ObjectId uncertaintyObjid;
    SObjectId::ObjectId configurationObjid;
    SObjectId::ObjectId triggerSettingObjid;
    char userDescription[ESS_USER_DESCRIPTION_MAX];
  };

  struct SEssDscField
  {
    CIOCommon *io;
    io_ddata_t *data;
  };

  struct SEssDscCb
  {
    uint8_t kind;
    uint8_t len;
    uint8_t data[ESS_RAW_DESCRIPTOR_MAX];
    SEssDscField field;
    SEssDscField measurement[6];
  };

  std::map<const SObjectId::ObjectId, SEssMeta> ioMetaMap; ///< Optional descriptor metadata.

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

  int allocESS();
  int createESS();
  void deleteESS();
  void configureDesc(const SObjectCfg::SObjectCfgItem *item);
  void allocObject(const SProtoNimblePrphIOBindEssObjid &obj, const uint32_t *ext);
  int configureDescriptors(struct ble_gatt_chr_def *chr, SObjectId::ObjectId objid, uint8_t type);
};
} // Namespace dawn
