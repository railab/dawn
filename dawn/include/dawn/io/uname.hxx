// dawn/include/dawn/io/uname.hxx
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
 * @brief System identification I/O providing hostname and version info.
 *
 * Provides read-only access to system identification information including
 * hostname and version strings.
 */

class CIOUname : public CIOCommon
{
public:
  explicit CIOUname(CDescObject &desc)
    : CIOCommon(desc)
    , dim(getDataDim(getCls()))
  {
  }

  ~CIOUname() override = default;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "hostname";
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

  constexpr static SObjectId::ObjectId objectIdHostname()
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_IO, IO_CLASS_SYSTEM_HOSTNAME, SObjectId::DTYPE_CHAR, 0, 0);
  }

private:
  size_t dim; ///< String data dimension.

  size_t getDataDim(uint16_t cls) const;
};
} // Namespace dawn
