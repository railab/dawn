// dawn/include/dawn/proto/nimble/prph.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/proto/nimble/adv.hxx"
#include "dawn/proto/nimble/hci.hxx"
#include "dawn/proto/nimble/host.hxx"
#include "dawn/proto/nimble/iprph.hxx"
#include "host/ble_gap.h"
#include "host/ble_gatt.h"

namespace dawn
{
/**
 * @brief BLE Peripheral (Slave) Protocol Implementation using NimBLE.
 *
 * This class implements the BLE peripheral role using Apache NimBLE stack.
 */

class CProtoNimblePrph
  : public CProtoCommon
  , public IProtoNimblePrphCb
{
public:
  enum
  {
    PROTO_NIMBLE_CFG_FIRST = 0,
    PROTO_NIMBLE_CFG_GAPNAME = 1,       ///< GAP device name configuration.
    PROTO_NIMBLE_CFG_IOBIND_DIS = 2,    ///< Device Information Service binding.
    PROTO_NIMBLE_CFG_IOBIND_BAS = 3,    ///< Battery Service binding.
    PROTO_NIMBLE_CFG_IOBIND_AIOS = 4,   ///< Automation I/O Service binding.
    PROTO_NIMBLE_CFG_IOBIND_ESS = 5,    ///< Environmental Sensing Service binding.
    PROTO_NIMBLE_CFG_IOBIND_IMDS = 6,   ///< Industrial Measurement Device Service binding.
    PROTO_NIMBLE_CFG_IOBIND_OTS = 7,    ///< Object Transfer Service binding.
    PROTO_NIMBLE_CFG_IOBIND_CUSTOM = 8, ///< Generic custom service binding.
    PROTO_NIMBLE_CFG_LAST = 31
  } typedef EProtoNimblePrphCfg;

  explicit CProtoNimblePrph(CDescObject &desc)
    : CProtoCommon(desc)
    , devno(0)
    , gapname(nullptr)
    , svcDefs(nullptr)
    , noAllocServices(0)
  {
  }

  ~CProtoNimblePrph() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "ble";
  }
#endif

  int configure() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;

  int serviceRegister(struct ble_gatt_svc_def *svc) override;
  int startService(int id) override;
  int stopService(int id) override;

  void regObject(SObjectId::ObjectId id) override
  {
    this->setObjectMapItem(id, nullptr);
  };

  CIOCommon *getObject(SObjectId::ObjectId id) override
  {
    return this->getIO(id);
  };

  size_t getObjectsLen() override
  {
    return this->getIOMap().size();
  };

  static SObjectId::ObjectId objectId(uint16_t id)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_NIMBLE_PRPH, SObjectId::DTYPE_ANY, 0, id);
  }

  static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_NIMBLE_PRPH, dtype, rw, size, id);
  }

  static SObjectCfg::ObjectCfgId cfgIdGapname(uint16_t size)
  {
    return CProtoNimblePrph::cfgId(false, SObjectId::DTYPE_CHAR, size, PROTO_NIMBLE_CFG_GAPNAME);
  }

  static SObjectCfg::ObjectCfgId cfgIdIOBindDis()
  {
    return CProtoNimblePrph::cfgId(false, SObjectId::DTYPE_ANY, 0, PROTO_NIMBLE_CFG_IOBIND_DIS);
  }

  static SObjectCfg::ObjectCfgId cfgIdIOBindBas()
  {
    return CProtoNimblePrph::cfgId(false, SObjectId::DTYPE_ANY, 1, PROTO_NIMBLE_CFG_IOBIND_BAS);
  }

  static SObjectCfg::ObjectCfgId cfgIdIOBindAios(uint16_t size)
  {
    return CProtoNimblePrph::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_NIMBLE_CFG_IOBIND_AIOS);
  }

  static SObjectCfg::ObjectCfgId cfgIdIOBindEss(uint16_t size)
  {
    return CProtoNimblePrph::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_NIMBLE_CFG_IOBIND_ESS);
  }

  static SObjectCfg::ObjectCfgId cfgIdIOBindImds(uint16_t size)
  {
    return CProtoNimblePrph::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_NIMBLE_CFG_IOBIND_IMDS);
  }

  static SObjectCfg::ObjectCfgId cfgIdIOBindOts(uint16_t size)
  {
    return CProtoNimblePrph::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_NIMBLE_CFG_IOBIND_OTS);
  }

  static SObjectCfg::ObjectCfgId cfgIdIOBindCustom(uint16_t size)
  {
    return CProtoNimblePrph::cfgId(
      false, SObjectId::DTYPE_ANY, size, PROTO_NIMBLE_CFG_IOBIND_CUSTOM);
  }

private:
  int devno;                                        ///< NimBLE device instance number.
  char *gapname;                                    ///< Advertised GAP device name.
  std::vector<IProtoNimblePrphService *> vservices; ///< Allocated service instances.
  std::vector<struct ble_gatt_svc_def *> vSvcDefs;  ///< GATT service definitions.
  std::vector<bool> vstart;                         ///< Service start/stop state flags.
  struct ble_gatt_svc_def *svcDefs; ///< Consolidated GATT service definitions array.
  uint8_t noAllocServices;          ///< Number of allocated services.
  CProtoNimbleAdv adv;              ///< BLE advertisement manager.
  CProtoNimbleHci hci;              ///< HCI manager.
  CProtoNimbleHost host;            ///< BLE host stack manager.

  int configureDesc(const CDescObject &desc);
  void servicesDefault();
  int servicesCreate();
  int servicesCount();
  int servicesInit();

  static void bleSyncCb();
};
} // Namespace dawn
