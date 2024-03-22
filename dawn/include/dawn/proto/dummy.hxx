// dawn/include/dawn/proto/dummy.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"

namespace dawn
{
/**
 * @brief Dummy Protocol for tests and binding placeholders.
 *
 * No-op Protocol that can be created by the protocol factory and can bind IOs
 * through the inherited CBindableObject interface.
 */

class CProtoDummy : public CProtoCommon
{
public:
  enum
  {
    PROTO_DUMMY_CFG_FIRST = 0,
    PROTO_DUMMY_CFG_IOBIND = 1, ///< I/O binding configuration.
    PROTO_DUMMY_CFG_LAST = 31
  };

  explicit CProtoDummy(CDescObject &desc)
    : CProtoCommon(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "proto_dummy";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_DUMMY, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROTO,
                                 CProtoCommon::PROTO_CLASS_DUMMY,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 0)
  {
    return CProtoDummy::cfgId(false, size, PROTO_DUMMY_CFG_IOBIND);
  }

  int configure() override;
};
} // Namespace dawn
