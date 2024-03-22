// dawn/include/dawn/proto/nxscope/serial.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/proto/nxscope/nxscope.hxx"

namespace dawn
{
/**
 * @brief NXScope over serial port transport.
 *
 * CProtoNxscopeSerial implements NXScope real-time data visualization over
 * serial connections (USB, UART).
 */

class CProtoNxscopeSerial : public CProtoNxscope
{
public:
  enum
  {
    PROTO_NXSCOPE_SERIAL_CFG_FIRST = 0,
    PROTO_NXSCOPE_SERIAL_CFG_IOBIND = 1,  ///< I/O bindings.
    PROTO_NXSCOPE_SERIAL_CFG_IOBIND2 = 2, ///< I/O bindings with names.
    PROTO_NXSCOPE_SERIAL_CFG_PATH = 4,    ///< Serial device path.
    PROTO_NXSCOPE_SERIAL_CFG_BAUD = 5,    ///< Baud rate.
    PROTO_NXSCOPE_SERIAL_CFG_LAST = 31
  };

  explicit CProtoNxscopeSerial(CDescObject &desc)
    : CProtoNxscope(desc)
    , baud(CONFIG_DAWN_PROTO_NXSCOPE_SERIAL_BAUD)
    , path(CONFIG_DAWN_PROTO_NXSCOPE_SERIAL_PATH)
    , serInit(false)
  {
  }

  ~CProtoNxscopeSerial() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "nxscope";
  }
#endif

  int initPriv() override;
  int deinitPriv() override;
  int startPriv() override;
  int stopPriv() override;

  constexpr static SObjectId::ObjectId objectId(uint16_t id)
  {
    return SObjectId::objectId(SObjectId::OBJTYPE_PROTO,
                               CProtoCommon::PROTO_CLASS_NXSCOPE_SERIAL,
                               SObjectId::DTYPE_ANY,
                               0,
                               id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_NXSCOPE_SERIAL, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoNxscopeSerial::cfgId(
      false, SObjectId::DTYPE_ANY, size, PROTO_NXSCOPE_SERIAL_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind2(uint16_t count)
  {
    return CProtoNxscopeSerial::cfgId(false,
                                      SObjectId::DTYPE_ANY,
                                      (count * (sizeof(CProtoNxscope::SProtoNxscopeIOBind2) / 4)),
                                      PROTO_NXSCOPE_SERIAL_CFG_IOBIND2);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPath(uint16_t size)
  {
    return CProtoNxscopeSerial::cfgId(
      false, SObjectId::DTYPE_CHAR, size, PROTO_NXSCOPE_SERIAL_CFG_PATH);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdBaud()
  {
    return CProtoNxscopeSerial::cfgId(
      false, SObjectId::DTYPE_UINT32, 1, PROTO_NXSCOPE_SERIAL_CFG_BAUD);
  }

private:
  uint32_t baud;                      ///< Serial baud rate.
  const char *path;                   ///< Serial device path.
  struct nxscope_ser_cfg_s nxsSerCfg; ///< NXScope serial transport configuration.
  bool serInit;                       ///< True when nxscope_ser_init succeeded.

  int configureDesc(const CDescObject &desc);
  int confgureNxscopePriv();
};
} // namespace dawn
