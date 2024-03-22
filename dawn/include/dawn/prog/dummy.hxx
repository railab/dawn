// dawn/include/dawn/prog/dummy.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"

namespace dawn
{
/**
 * @brief Dummy Program for tests and descriptor binding placeholders.
 *
 * No-op Program that can be created by the protocol factory and can bind IOs
 * through the inherited CBindableObject interface.
 */

class CProgDummy : public CProgCommon
{
public:
  enum
  {
    PROG_DUMMY_CFG_FIRST = 0,
    PROG_DUMMY_CFG_IOBIND = 1, ///< I/O binding configuration.
    PROG_DUMMY_CFG_LAST = 31
  };

  explicit CProgDummy(CDescObject &desc)
    : CProgCommon(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "prog_dummy";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_DUMMY, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_DUMMY, SObjectId::DTYPE_ANY, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 0)
  {
    return CProgDummy::cfgId(false, size, PROG_DUMMY_CFG_IOBIND);
  }

  int configure() override;
};
} // Namespace dawn
