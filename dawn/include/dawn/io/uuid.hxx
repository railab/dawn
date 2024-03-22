// dawn/include/dawn/io/uuid.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief System UUID I/O providing unique device identifier.
 *
 * Provides read-only access to the device's unique identifier (UUID)
 * using the board_uniqueid() system interface. The UUID length is
 * determined at compile-time by CONFIG_BOARDCTL_UNIQUEID_SIZE.
 */

class CIOUuid : public CIOCommon
{
public:
  explicit CIOUuid(CDescObject &desc)
    : CIOCommon(desc)
  {
  }

  ~CIOUuid() override = default;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "uuid";
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

  constexpr static SObjectId::ObjectId objectId(uint8_t instance)
  {
    return SObjectId::objectId(SObjectId::OBJTYPE_IO,
                               IO_CLASS_SYSTEM_UUID,
                               SObjectId::DTYPE_UINT8,
                               instance,
                               CONFIG_BOARDCTL_UNIQUEID_SIZE);
  }
};
} // Namespace dawn
