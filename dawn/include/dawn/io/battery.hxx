// dawn/include/dawn/io/battery.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <limits.h>

#include "dawn/io/common.hxx"

namespace dawn
{
/** @brief Shared base for the battery (fuel-gauge) IO family. */

class CIOBatteryBase : public CIOCommon
{
public:
  explicit CIOBatteryBase(CDescObject &desc)
    : CIOCommon(desc)
    , fd(-1)
  {
  }

  ~CIOBatteryBase() override;

  int configure() override;
  int deinit() override;

#ifdef CONFIG_DAWN_IO_NOTIFY
  int getFd() const override
  {
    return fd;
  }
#endif

  size_t getDataSize() const override
  {
    return getDtypeSize();
  }

  size_t getDataDim() const override
  {
    return 1;
  }

  bool isRead() const override
  {
    return true;
  }

  bool isWrite() const override
  {
    return false;
  }

  bool isNotify() const override
  {
    return false;
  }

  bool isBatch() const override
  {
    return false;
  }

protected:
  int getFdInternal() const
  {
    return fd;
  }

  /** @brief ioctl read of a single battery value into *arg. */

  int batRead(int cmd, void *arg) const;

private:
  char path[PATH_MAX] = {};
  int fd;
};

/** @brief Battery voltage in mV (DTYPE_UINT32). */

class CIOBattVolt : public CIOBatteryBase
{
public:
  explicit CIOBattVolt(CDescObject &desc)
    : CIOBatteryBase(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "battery_volt";
  }
#endif

  int getDataImpl(IODataCmn &data, size_t len) override;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return CIOCommon::IOObjectIdHelper<IO_CLASS_BATTERY_VOLTAGE, SObjectId::DTYPE_UINT32>::create(
      ts, inst);
  }
};

/** @brief Battery state of charge in percent (DTYPE_UINT32). */

class CIOBattSoc : public CIOBatteryBase
{
public:
  explicit CIOBattSoc(CDescObject &desc)
    : CIOBatteryBase(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "battery_soc";
  }
#endif

  int getDataImpl(IODataCmn &data, size_t len) override;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return CIOCommon::IOObjectIdHelper<IO_CLASS_BATTERY_SOC, SObjectId::DTYPE_UINT32>::create(ts,
                                                                                              inst);
  }
};

/** @brief Battery charge state as a numeric code (BATTERY_*; DTYPE_UINT32). */

class CIOBattState : public CIOBatteryBase
{
public:
  explicit CIOBattState(CDescObject &desc)
    : CIOBatteryBase(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "battery_state";
  }
#endif

  int getDataImpl(IODataCmn &data, size_t len) override;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return CIOCommon::IOObjectIdHelper<IO_CLASS_BATTERY_STATE, SObjectId::DTYPE_UINT32>::create(
      ts, inst);
  }
};

} // namespace dawn
