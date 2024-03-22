// dawn/include/dawn/io/boardctl.hxx
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
 * @brief Board control I/O for system operations.
 *
 * Provides hardware-level board control operations including system reset,
 * reset cause inquiry, and power-off.
 */

class CIOBoardctl : public CIOCommon
{
public:
  explicit CIOBoardctl(CDescObject &desc)
    : CIOCommon(desc)
    , dim(getDataDim(getCls()))
  {
  }

  ~CIOBoardctl() override = default;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    switch (getCls())
      {
        case IO_CLASS_SYSTEM_RESET:
          return "reset";
        case IO_CLASS_SYSTEM_RESETCAUSE:
          return "resetcause";
        case IO_CLASS_SYSTEM_POWEROFF:
          return "poweroff";
        default:
          return "boardctl";
      }
  }
#endif

  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;
  size_t getDataSize() const override;
  size_t getDataDim() const override;
  bool isRead() const override;
  bool isWrite() const override;

  bool isNotify() const override
  {
    return false;
  };

  bool isBatch() const override
  {
    return false;
  };

#ifdef CONFIG_BOARDCTL_RESET
  constexpr static SObjectId::ObjectId objectIdReset()
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_IO, IO_CLASS_SYSTEM_RESET, SObjectId::DTYPE_INT32, 0, 0);
  }
#endif

#ifdef CONFIG_BOARDCTL_RESET_CAUSE
  constexpr static SObjectId::ObjectId objectIdResetCause()
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_IO, IO_CLASS_SYSTEM_RESETCAUSE, SObjectId::DTYPE_UINT32, 0, 0);
  }
#endif

#ifdef CONFIG_BOARDCTL_POWEROFF
  constexpr static SObjectId::ObjectId objectIdPoweroff()
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_IO, IO_CLASS_SYSTEM_POWEROFF, SObjectId::DTYPE_INT32, 0, 0);
  }
#endif

private:
  size_t dim;

  size_t getDataDim(uint16_t cls) const;
};
} // Namespace dawn
