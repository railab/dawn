// dawn/include/dawn/io/trigger.hxx
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
/** @brief Trigger I/O for dispatching commands to bound objects. */

class CIOTrigger : public CIOCommon
{
public:
  enum
  {
    IO_TRIGGER_CFG_FIRST = 0,
    IO_TRIGGER_CFG_ALLOCOBJ = 1, ///< Bound object ID list.
    IO_TRIGGER_CFG_ALLOWED = 2,  ///< Allowed commands bitmask.
    IO_TRIGGER_CFG_LAST = 31
  };

  enum
  {
    TRIG_ALLOW_RESET = (1 << 0),    ///< Allow CMD_RESET command.
    TRIG_ALLOW_TRIGGER1 = (1 << 1), ///< Allow CMD_TRIGGER1 command.
    TRIG_ALLOW_TRIGGER2 = (1 << 2), ///< Allow CMD_TRIGGER2 command.
    TRIG_ALLOW_TRIGGER3 = (1 << 3)  ///< Allow CMD_TRIGGER3 command.
  };

  explicit CIOTrigger(CDescObject &desc)
    : CIOCommon(desc)
    , allowed(0)
  {
  }

  ~CIOTrigger() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "trigger";
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
    return false;
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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelperNoTS<IO_CLASS_TRIGGER, SObjectId::DTYPE_UINT8>;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return ObjectIdHelper::create(inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAllocObj(size_t n = 1)
  {
    return CIOTrigger::cfgId(false, static_cast<uint8_t>(n), IO_TRIGGER_CFG_ALLOCOBJ);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAllowed()
  {
    return CIOTrigger::cfgId(false, 1, IO_TRIGGER_CFG_ALLOWED);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_TRIGGER, SObjectId::DTYPE_UINT8, rw, size, id);
  }

private:
  uint32_t allowed;               ///< Bitmask of allowed commands (EIOTriggerAllow values).
  std::vector<CObject *> targets; ///< Resolved target objects; populated by bind().

  int configureDesc(const CDescObject &desc);
};

} // namespace dawn
