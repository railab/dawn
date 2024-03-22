// dawn/include/dawn/proto/nxscope/dummy.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/proto/nxscope/nxscope.hxx"

namespace dawn
{
/**
 * @brief NXScope no-op/dummy transport implementation.
 *
 * CProtoNxscopeDummy implements a null transport backend for NXScope protocol.
 */

class CProtoNxscopeDummy : public CProtoNxscope
{
public:
  enum
  {
    PROTO_NXSCOPE_DUMMY_CFG_FIRST = 0,
    PROTO_NXSCOPE_DUMMY_CFG_IOBIND = 1,  ///< I/O bindings.
    PROTO_NXSCOPE_DUMMY_CFG_IOBIND2 = 2, ///< I/O bindings with names.
    PROTO_NXSCOPE_DUMMY_CFG_LAST = 31
  };

  explicit CProtoNxscopeDummy(CDescObject &desc)
    : CProtoNxscope(desc)
  {
  }

  ~CProtoNxscopeDummy() override;

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
                               CProtoCommon::PROTO_CLASS_NXSCOPE_DUMMY,
                               SObjectId::DTYPE_ANY,
                               0,
                               id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_NXSCOPE_DUMMY, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return CProtoNxscopeDummy::cfgId(
      false, SObjectId::DTYPE_ANY, size, PROTO_NXSCOPE_DUMMY_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind2(uint16_t count)
  {
    return CProtoNxscopeDummy::cfgId(false,
                                     SObjectId::DTYPE_ANY,
                                     (count * (sizeof(CProtoNxscope::SProtoNxscopeIOBind2) / 4)),
                                     PROTO_NXSCOPE_DUMMY_CFG_IOBIND2);
  }

private:
  struct nxscope_dummy_cfg_s nxsDummyCfg; ///< NXScope dummy transport configuration.

  int configureDesc(const CDescObject &desc);
  int confgureNxscopePriv();
};
} // namespace dawn
