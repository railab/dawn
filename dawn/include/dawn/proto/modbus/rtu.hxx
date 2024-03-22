// dawn/include/dawn/proto/modbus/rtu.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/thread.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"
#include "dawn/proto/modbus/regs.hxx"
#include <nxmodbus/nxmodbus.h>

namespace dawn
{
/**
 * @brief Modbus RTU (Remote Terminal Unit) protocol implementation.
 *
 * CProtoModbusRtu implements the Modbus RTU protocol over serial lines,
 * providing industrial-standard register mapping for I/O objects.
 */

class CProtoModbusRtu
  : public CProtoCommon
  , public CProtoModbusRegs
  , protected CThreadedObject
{
public:
  enum
  {
    PROTO_MODBUS_CFG_FIRST = 0,
    PROTO_MODBUS_CFG_IOBIND = 1, ///< I/O object binding (register map).
    PROTO_MODBUS_CFG_PATH = 2,   ///< Serial device path.
    PROTO_MODBUS_CFG_BAUD = 3,   ///< Serial baud rate.
    PROTO_MODBUS_CFG_LAST = 31
  };

  explicit CProtoModbusRtu(CDescObject &desc)
    : CProtoCommon(desc)
    , mbhandle(nullptr)
    , callbacks()
    , saddr(CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR)
    , parity(CONFIG_DAWN_PROTO_MODBUS_RTU_PARITY)
    , baud(CONFIG_DAWN_PROTO_MODBUS_RTU_BAUD)
    , path(CONFIG_DAWN_PROTO_MODBUS_RTU_PATH)
  {
  }

  ~CProtoModbusRtu() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "modbus_rtu";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;

  constexpr static SObjectId::ObjectId objectId(uint16_t id)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_MODBUS_RTU, SObjectId::DTYPE_ANY, 0, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_MODBUS_RTU, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoModbusRtu::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_MODBUS_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPath(uint16_t size)
  {
    return CProtoModbusRtu::cfgId(true, SObjectId::DTYPE_CHAR, size, PROTO_MODBUS_CFG_PATH);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdBaud()
  {
    return CProtoModbusRtu::cfgId(true, SObjectId::DTYPE_UINT32, 1, PROTO_MODBUS_CFG_BAUD);
  }

private:
  nxmb_handle_t mbhandle;
  struct nxmb_callbacks_s callbacks;
  uint8_t saddr;
  uint8_t parity;
  uint32_t baud;
  const char *path;

  int configureDesc(const CDescObject &desc);
  void allocObject(SProtoModbusIOBind *alloc);
  void thread();
  int modbusInitialize();

  CIOCommon *getIO_(SObjectId::ObjectId id) override
  {
    return this->getIO(id);
  };
};

} // Namespace dawn
