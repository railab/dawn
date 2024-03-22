// dawn/include/dawn/io/gpi.hxx
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
 * @brief GPIO Input (GPI) I/O type for reading digital input states.
 *
 * Provides interface for reading digital input values from GPIO pins and
 * similar devices.
 */

class CIOGpi : public CIOCommon
{
public:
  enum
  {
    IO_GPI_CFG_FIRST = 0,
    IO_GPI_CFG_LAST = 31
  };

  explicit CIOGpi(CDescObject &desc)
    : CIOCommon(desc)
    , path()
    , fd(-1)
  {
  }

  ~CIOGpi() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "gpi";
  }
#endif

  int configure() override;
  int init() override;
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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_GPI_SINGLE, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

private:
  char path[PATH_MAX]; ///< GPI device file path.
  int fd;              ///< File descriptor for GPI device.
#ifdef CONFIG_DAWN_IO_NOTIFY
  bool isNotifyIO;     ///< True if this GPI supports notifications.
#endif

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
