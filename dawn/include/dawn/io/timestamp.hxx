// dawn/include/dawn/io/timestamp.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <mutex>

#include "dawn/io/common.hxx"
#include "dawn/io/timerfd.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief System timestamp/monotonic time generator.
 *
 * Provides periodic timestamp data from the system clock.
 */

class CIOTimestamp
  : public CIOCommon
  , public CIOTimerfd
{
public:
  enum
  {
    IO_TIMESTAMP_CFG_FIRST = 0,
    IO_TIMESTAMP_CFG_INTERVAL = 1,
    IO_TIMESTAMP_CFG_LAST = 31
  };

  explicit CIOTimestamp(CDescObject &desc)
    : CIOCommon(desc)
    , tlen(getDtypeSize())
  {
  }

  ~CIOTimestamp() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "timestamp";
  }
#endif

  int doStart() override;
  int doStop() override;
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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_TIMESTAMP, SObjectId::DTYPE_UINT64>;

  constexpr static SObjectId::ObjectId objectId(SObjectId::EObjectDataType dtype,
                                                bool ts,
                                                uint16_t inst)
  {
    return ObjectIdHelper::create(dtype, ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgInterval(bool rw)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_TIMESTAMP,
                                 SObjectId::DTYPE_UINT32,
                                 rw,
                                 1,
                                 IO_TIMESTAMP_CFG_INTERVAL);
  }

private:
  size_t tlen; ///< Type size in bytes.

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
