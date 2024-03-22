// dawn/include/dawn/proto/nxscope/udp.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/proto/nxscope/nxscope.hxx"

namespace dawn
{
/** @brief NXScope over UDP transport */

class CProtoNxscopeUdp : public CProtoNxscope
{
public:
  enum
  {
    PROTO_NXSCOPE_UDP_CFG_FIRST = 0,
    PROTO_NXSCOPE_UDP_CFG_IOBIND = 1,  ///< I/O bindings.
    PROTO_NXSCOPE_UDP_CFG_IOBIND2 = 2, ///< I/O bindings with names.
    PROTO_NXSCOPE_UDP_CFG_PORT = 6,    ///< UDP local port.
    PROTO_NXSCOPE_UDP_CFG_LAST = 31
  };

  explicit CProtoNxscopeUdp(CDescObject &desc)
    : CProtoNxscope(desc)
    , port(CONFIG_DAWN_PROTO_NXSCOPE_UDP_PORT)
  {
  }

  ~CProtoNxscopeUdp() override;

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
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_NXSCOPE_UDP, SObjectId::DTYPE_ANY, 0, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_NXSCOPE_UDP, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoNxscopeUdp::cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_NXSCOPE_UDP_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind2(uint16_t count)
  {
    return CProtoNxscopeUdp::cfgId(false,
                                   SObjectId::DTYPE_ANY,
                                   (count * (sizeof(CProtoNxscope::SProtoNxscopeIOBind2) / 4)),
                                   PROTO_NXSCOPE_UDP_CFG_IOBIND2);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPort()
  {
    return CProtoNxscopeUdp::cfgId(false, SObjectId::DTYPE_UINT16, 1, PROTO_NXSCOPE_UDP_CFG_PORT);
  }

private:
  uint16_t port;                      ///< UDP local port.
  struct nxscope_udp_cfg_s nxsUdpCfg; ///< NXScope UDP transport configuration.

  int configureDesc(const CDescObject &desc);
  int confgureNxscopePriv();
};
} // namespace dawn
