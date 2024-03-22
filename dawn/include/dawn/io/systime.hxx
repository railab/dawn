// dawn/include/dawn/io/systime.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"

namespace dawn
{
/** @brief System information I/O providing system time */

class CIOSystime : public CIOCommon
{
public:
  explicit CIOSystime(CDescObject &desc)
    : CIOCommon(desc)
  {
  }

  ~CIOSystime() override = default;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "systime";
  }
#endif

  int init() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;
  size_t getDataSize() const override;
  size_t getDataDim() const override;
  bool isRead() const override;
  bool isWrite() const override;
  bool isNotify() const override;
  bool isBatch() const override;

  constexpr static SObjectId::ObjectId objectId(uint8_t instance)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_IO, IO_CLASS_SYSTEM_SYSTEMTIME, SObjectId::DTYPE_UINT64, instance, 1);
  }
};

}; // namespace dawn
