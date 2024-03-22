// dawn/include/dawn/io/sysinfo.hxx
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
 * @brief System information I/O providing uptime and CPU load.
 *
 * Provides read-only access to system-level information including system
 * uptime (elapsed time since boot) and CPU load metrics.
 */

class CIOSysinfo : public CIOCommon
{
public:
  explicit CIOSysinfo(CDescObject &desc)
    : CIOCommon(desc)
    , dim(getDataDim(getCls()))
  {
  }

  ~CIOSysinfo() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    switch (getCls())
      {
        case IO_CLASS_SYSTEM_UPTIME:
          return "uptime";
        case IO_CLASS_SYSTEM_CPULOAD:
          return "cpuload";
        default:
          return "sysinfo";
      }
  }
#endif

  int init() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;
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
    return false;
  };

  bool isBatch() const override
  {
    return false;
  };

  constexpr static SObjectId::ObjectId objectIdUptime()
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_IO, IO_CLASS_SYSTEM_UPTIME, SObjectId::DTYPE_UINT64, 0, 1);
  }

  constexpr static SObjectId::ObjectId objectIdCpuload(uint8_t dtype)
  {
    return SObjectId::objectId(SObjectId::OBJTYPE_IO, IO_CLASS_SYSTEM_CPULOAD, dtype, 0, 1);
  }

private:
  size_t dim; ///< Data dimensionality for this system info type.

  size_t getDataDim(uint16_t cls) const;
};
} // Namespace dawn
