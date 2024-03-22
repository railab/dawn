// dawn/include/dawn/io/gpo.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/porting/gpio.hxx"

namespace dawn
{
/**
 * @brief GPIO Output (GPO) I/O type for writing digital output states.
 *
 * Provides interface for writing digital output values to GPIO pins and
 * similar devices.
 */

class CIOGpo : public CIOCommon
{
public:
  enum
  {
    IO_GPO_CFG_FIRST = 0,
    IO_GPO_CFG_LAST = 31
  };

  explicit CIOGpo(CDescObject &desc)
    : CIOCommon(desc)
    , path()
    , fd(-1)
#ifdef CONFIG_DAWN_IO_TIMESTAMP
    , ts(0)
#endif
  {
  }

  ~CIOGpo() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "gpo";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;
  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return false;
  };

  bool isWrite() const override
  {
    return true;
  };

  bool isNotify() const override
  {
    return false;
  };

  bool isBatch() const override
  {
    return false;
  };

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_GPO_SINGLE, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

private:
  char path[PATH_MAX]; ///< GPO device file path.
  int fd;              ///< File descriptor for GPO device.
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;         ///< Data update timestamp.
#endif

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
