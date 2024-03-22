// dawn/include/dawn/io/control.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/common/object.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/** @brief Control I/O for lifecycle management of bound objects. */

class CIOControl : public CIOCommon
{
public:
  enum
  {
    IO_CONTROL_CFG_FIRST = 0,
    IO_CONTROL_CFG_ALLOCOBJ = 1,
    IO_CONTROL_CFG_ALLOWED = 2,
    IO_CONTROL_CFG_LAST = 31
  };

  enum
  {
    CTRL_ALLOW_STOP = (1 << 0), ///< Allow stop command.
    CTRL_ALLOW_START = (1 << 1) ///< Allow start command.
  };

  explicit CIOControl(CDescObject &desc)
    : CIOCommon(desc)
    , allowed(0)
  {
  }

  ~CIOControl() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "control";
  }
#endif

  int configure() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;
  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return true;
  }

  bool isWrite() const override
  {
    return true;
  }

  bool isNotify() const override
  {
    return false;
  }

  bool isBatch() const override
  {
    return false;
  }

  int bind(CObject *obj);

  std::vector<SObjectId::ObjectId> ids; ///< Object IDs to resolve; populated during configure().

  using ObjectIdHelper = CIOCommon::IOObjectIdHelperNoTS<IO_CLASS_CONTROL, SObjectId::DTYPE_UINT8>;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return ObjectIdHelper::create(inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAllocObj(size_t n = 1)
  {
    return CIOControl::cfgId(false, static_cast<uint8_t>(n), IO_CONTROL_CFG_ALLOCOBJ);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAllowed()
  {
    return CIOControl::cfgId(false, 1, IO_CONTROL_CFG_ALLOWED);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_CONTROL, SObjectId::DTYPE_UINT8, rw, size, id);
  }

private:
  std::vector<CObject *> targets; ///< Resolved target objects; populated by bind().
  uint32_t allowed;               ///< Bitmask of allowed commands (EIOControlAllow values).

  int configureDesc(const CDescObject &desc);
};

} // namespace dawn
