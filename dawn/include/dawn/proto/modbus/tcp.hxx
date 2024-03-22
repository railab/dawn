// dawn/include/dawn/proto/modbus/tcp.hxx
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
 * @brief Modbus TCP  protocol implementation.
 *
 * CProtoModbusTcp implements the Modbus TCP protocol over network,
 * providing industrial-standard register mapping for I/O objects.
 */

class CProtoModbusTcp
  : public CProtoCommon
  , public CProtoModbusRegs
  , protected CThreadedObject
{
public:
  enum
  {
    PROTO_MODBUS_TCP_CFG_FIRST = 0,
    PROTO_MODBUS_TCP_CFG_IOBIND = 1,
    PROTO_MODBUS_TCP_CFG_PORT = 2,
    PROTO_MODBUS_TCP_CFG_LAST = 31
  };

  explicit CProtoModbusTcp(CDescObject &desc)
    : CProtoCommon(desc)
    , mbhandle(nullptr)
    , callbacks()
    , saddr(CONFIG_DAWN_PROTO_MODBUS_TCP_ADDR)
    , port(CONFIG_DAWN_PROTO_MODBUS_TCP_PORT)
  {
  }

  ~CProtoModbusTcp() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "modbus_tcp";
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
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_MODBUS_TCP, SObjectId::DTYPE_ANY, 0, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_MODBUS_TCP, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoModbusTcp::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_MODBUS_TCP_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPort()
  {
    return CProtoModbusTcp::cfgId(true, SObjectId::DTYPE_UINT16, 1, PROTO_MODBUS_TCP_CFG_PORT);
  }

private:
  nxmb_handle_t mbhandle;
  struct nxmb_callbacks_s callbacks;
  uint8_t saddr;
  uint16_t port;

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
