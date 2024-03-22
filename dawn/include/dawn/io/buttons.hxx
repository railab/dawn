// dawn/include/dawn/io/buttons.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <mutex>

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Button input I/O type for reading button/switch states.
 *
 * Provides interface for reading digital button and switch states from input
 * devices.
 */

class CIOButtons : public CIOCommon
{
public:
  enum
  {
    IO_BUTTONS_CFG_FIRST = 0,
    IO_BUTTONS_CFG_LAST = 31
  };

  explicit CIOButtons(CDescObject &desc)
    : CIOCommon(desc)
    , fd(-1)
  {
  }

  ~CIOButtons() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "button";
  }
#endif

  int configure() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;

#ifdef CONFIG_DAWN_IO_NOTIFY
  int getFd() const override;
#endif

  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return true;
  };

  bool isWrite() const override
  {
    return false;
  };

  bool isNotify() const override
  {
    return true;
  };

  bool isBatch() const override
  {
    return false;
  };

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_BUTTONS, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_BUTTONS, dtype, rw, size, id);
  }

private:
  char path[PATH_MAX] = {};
  int fd;

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
