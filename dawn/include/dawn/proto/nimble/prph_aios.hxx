// dawn/include/dawn/proto/nimble/prph_aios.hxx
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
 * @brief Automation I/O Service (AIOS) for BLE Peripheral.
 *
 * This class implements the Bluetooth SIG Automation I/O Service (0x1815).
 */

class CProtoNimblePrphAios : public IProtoNimblePrphService
{
public:
  constexpr static uint8_t AIOS_CFG0_SIZE_SHIFT = 0;
  constexpr static uint8_t AIOS_CFG0_SIZE_MASK = 0b1111;
  constexpr static uint8_t AIOS_CFG1_AGGREGATE_SHIFT = 0;
  constexpr static uint8_t AIOS_CFG1_AGGREGATE_MASK = 0b1;
  constexpr static uint8_t AIOS_OBJCFG_TYPE_SHIFT = 0;
  constexpr static uint8_t AIOS_OBJCFG_TYPE_MASK = 0b11;
  constexpr static uint8_t AIOS_EXTCFG_KIND_SHIFT = 0;
  constexpr static uint8_t AIOS_EXTCFG_KIND_MASK = 0xff;
  constexpr static uint8_t AIOS_EXTCFG_SIZE_SHIFT = 8;
  constexpr static uint8_t AIOS_EXTCFG_SIZE_MASK = 0xff;
  constexpr static uint8_t AIOS_DESC_USER_DESCRIPTION = 1 << 0;
  constexpr static uint8_t AIOS_DESC_NUMBER_OF_DIGITALS = 1 << 1;
  constexpr static uint8_t AIOS_DESC_VALUE_TRIGGER_SETTING = 1 << 2;
  constexpr static uint8_t AIOS_DESC_TIME_TRIGGER_SETTING = 1 << 3;
  constexpr static uint8_t AIOS_DESC_PRESENTATION_FORMAT = 1 << 4;
  constexpr static uint8_t AIOS_DESC_EXTENDED_PROPERTIES = 1 << 5;
  constexpr static uint8_t AIOS_USER_DESCRIPTION_MAX = 16;
  constexpr static uint8_t AIOS_RAW_DESCRIPTOR_MAX = 16;
  constexpr static uint8_t AIOS_PRESENTATION_FORMAT_SIZE = 7;
  constexpr static uint8_t AIOS_EXTENDED_PROPERTIES_SIZE = 2;

  constexpr static uint16_t UUID16_EXTENDED_PROPERTIES = 0x2900;
  constexpr static uint16_t UUID16_PRESENTATION_FORMAT = 0x2904;
  constexpr static uint16_t UUID16_CHR_USER_DESCRIPTION = 0x2901;
  constexpr static uint16_t UUID16_NUMBER_OF_DIGITALS = 0x2909;
  constexpr static uint16_t UUID16_VALUE_TRIGGER_SETTING = 0x290a;
  constexpr static uint16_t UUID16_TIME_TRIGGER_SETTING = 0x290e;

  enum
  {
    PRPH_AIOS_TYPE_DIGITAL = 0, ///< Digital I/O (GPIO, buttons, LEDs).
    PRPH_AIOS_TYPE_ANALOG = 1   ///< Analog I/O (ADC, DAC).
  };

  enum
  {
    AIOS_EXT_USER_DESCRIPTION = 1,  ///< Characteristic User Description descriptor.
    AIOS_EXT_NUMBER_OF_DIGITALS,    ///< Number of Digitals descriptor.
    AIOS_EXT_VALUE_TRIGGER_SETTING, ///< Value Trigger Setting descriptor.
    AIOS_EXT_TIME_TRIGGER_SETTING,  ///< Time Trigger Setting descriptor.
    AIOS_EXT_PRESENTATION_FORMAT,   ///< Characteristic Presentation Format descriptor.
    AIOS_EXT_EXTENDED_PROPERTIES    ///< Characteristic Extended Properties descriptor.
  };

  struct
  {
    uint32_t cfg;               ///< I/O type (digital or analog).
    SObjectId::UObjectId objid; ///< I/O object ID for this characteristic.
    uint32_t extCount;          ///< Number of optional descriptor extension records.
  } typedef SProtoNimblePrphIOBindAiosObjid;

  struct
  {
    uint32_t cfg0;   ///< Config 0: number of I/Os.
    uint32_t cfg1;   ///< Config 1: reserved.
    uint32_t cfg2;   ///< Config 2: reserved.

    uint32_t data[]; ///< Variable-length I/O bindings and optional descriptor extensions.
  } typedef SProtoNimblePrphIOBindAios;

  CProtoNimblePrphAios(const SObjectCfg::SObjectCfgItem *item, IProtoNimblePrphCb *cb_);

  ~CProtoNimblePrphAios() override;

  int init() override;
  int deinit() override;
  int start() override;
  int stop() override;

  static uint32_t cfgIdIOBindAiosCfgObj(uint8_t type)
  {
    return ((type & AIOS_OBJCFG_TYPE_MASK) << AIOS_OBJCFG_TYPE_SHIFT);
  }

  static uint8_t cfgIdIOBindAiosCfgObjTypeGet(uint32_t val)
  {
    return (val >> AIOS_OBJCFG_TYPE_SHIFT) & AIOS_OBJCFG_TYPE_MASK;
  }

  static uint32_t cfgIdIOBindAiosExt(uint8_t kind, uint8_t size)
  {
    return ((kind & AIOS_EXTCFG_KIND_MASK) << AIOS_EXTCFG_KIND_SHIFT) |
           ((size & AIOS_EXTCFG_SIZE_MASK) << AIOS_EXTCFG_SIZE_SHIFT);
  }

  static uint8_t cfgIdIOBindAiosExtKindGet(uint32_t val)
  {
    return (val >> AIOS_EXTCFG_KIND_SHIFT) & AIOS_EXTCFG_KIND_MASK;
  }

  static uint8_t cfgIdIOBindAiosExtSizeGet(uint32_t val)
  {
    return (val >> AIOS_EXTCFG_SIZE_SHIFT) & AIOS_EXTCFG_SIZE_MASK;
  }

  static uint32_t cfgIdIOBindAiosCfg0(uint8_t size)
  {
    return ((size & AIOS_CFG0_SIZE_MASK) << AIOS_CFG0_SIZE_SHIFT);
  }

  static uint8_t cfgIdIOBindAiosCfg0SizeGet(uint32_t val)
  {
    return (val >> AIOS_CFG0_SIZE_SHIFT) & AIOS_CFG0_SIZE_MASK;
  }

  static uint32_t cfgIdIOBindAiosCfg1(uint8_t aggregate)
  {
    return ((aggregate & AIOS_CFG1_AGGREGATE_MASK) << AIOS_CFG1_AGGREGATE_SHIFT);
  }

  static bool cfgIdIOBindAiosCfg1AggregateGet(uint32_t val)
  {
    return ((val >> AIOS_CFG1_AGGREGATE_SHIFT) & AIOS_CFG1_AGGREGATE_MASK) != 0;
  }

  static uint32_t cfgIdIOBindAiosCfg2(uint8_t res)
  {
    return 0;
  }

private:
  int id;                      ///< Service ID assigned by peripheral during registration.
  size_t noChar;               ///< Number of I/O characteristics in this service.
  bool aggregateEnabled;       ///< Whether to expose the optional Aggregate characteristic.
  bool created;                ///< Whether runtime characteristic state has been built.
  struct ble_gatt_svc_def svc; ///< GATT service definition structure.
  std::map<const SObjectId::ObjectId, uint8_t> ioTypeMap; ///< Map of I/O object ID to I/O type.
  std::map<const SObjectId::ObjectId, SPrphNotiferCb *>
    ioCbMap; ///< Map of I/O object ID to GATT callback state.

  struct SAiosMeta
  {
    uint32_t desc;
    char userDescription[AIOS_USER_DESCRIPTION_MAX];
    uint8_t numberOfDigitals;
    SObjectId::ObjectId valueTriggerSettingObjid;
    SObjectId::ObjectId timeTriggerSettingObjid;
    uint8_t presentationFormat[AIOS_PRESENTATION_FORMAT_SIZE];
    uint16_t extendedProperties;
  };

  struct SAiosDscField
  {
    CIOCommon *io;
    io_ddata_t *data;
  };

  struct SAiosDscCb
  {
    uint8_t kind;
    uint8_t len;
    uint8_t data[AIOS_RAW_DESCRIPTOR_MAX];
    SAiosDscField field;
  };

  std::map<const SObjectId::ObjectId, SAiosMeta> ioMetaMap; ///< Optional descriptor metadata.

#ifdef CONFIG_DAWN_IO_NOTIFY
  static int notifierCb(void *priv, io_ddata_t *data);
#endif

  static int descriptorCb(uint16_t conn_handle,
                          uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt,
                          void *arg);

  static int callbackAnalog(uint16_t conn_handle,
                            uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt,
                            void *arg);

  static int callbackDigital(uint16_t conn_handle,
                             uint16_t attr_handle,
                             struct ble_gatt_access_ctxt *ctxt,
                             void *arg);

  static int callbackAggregate(uint16_t conn_handle,
                               uint16_t attr_handle,
                               struct ble_gatt_access_ctxt *ctxt,
                               void *arg);

  int appendAggregateValue(struct os_mbuf *om);
  int allocAIOS();
  int createAIOS();
  void deleteAIOS();
  void configureDesc(const SObjectCfg::SObjectCfgItem *item);
  void allocObject(const SProtoNimblePrphIOBindAiosObjid &obj, const uint32_t *ext);
  int configureDescriptors(struct ble_gatt_chr_def *chr, SObjectId::ObjectId objid);
};
} // Namespace dawn
