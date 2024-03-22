// dawn/include/dawn/io/dummy_notify.hxx
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
 * @brief Timer-driven dummy I/O with notification support.
 *
 * Stores a writable in-memory value like CIODummy, but uses a timerfd to emit
 * periodic notifications so programs that require notifier-backed inputs can
 * consume it.
 */

class CIODummyNotify
  : public CIOCommon
  , public CIOTimerfd
{
public:
  enum
  {
    IO_DUMMY_NOTIFY_CFG_FIRST = 0,
    IO_DUMMY_NOTIFY_CFG_INITVAL = 1,
    IO_DUMMY_NOTIFY_CFG_INTERVAL = 2,
    IO_DUMMY_NOTIFY_CFG_DIM = 3,
    IO_DUMMY_NOTIFY_CFG_NOTIFY_ON_WRITE = 4,
    IO_DUMMY_NOTIFY_CFG_LAST = 31
  };

  static_assert(IO_DUMMY_NOTIFY_CFG_LAST - 1 <= SObjectCfg::ID_MAX);

  explicit CIODummyNotify(CDescObject &desc)
    : CIOCommon(desc)
    , cfgval(nullptr)
    , val(nullptr)
    , writeData(nullptr)
    , notifyOnWrite(false)
    , dlen(1)
    , cfglen(1)
    , tlen(getDtypeSize())
#ifdef CONFIG_DAWN_IO_TIMESTAMP
    , ts(0)
#endif
  {
  }

  ~CIODummyNotify() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "dummy_notify";
  }
#endif

  int doStart() override;
  int doStop() override;
  int configure() override;
  int init() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;

#ifdef CONFIG_DAWN_IO_NOTIFY
  int getFd() const override;
  int notify();
#endif

  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return true;
  };

  bool isWrite() const override
  {
    return true;
  };

  bool isNotify() const override
  {
#ifdef CONFIG_DAWN_IO_NOTIFY
    return true;
#else
    return false;
#endif
  };

  bool isBatch() const override
  {
    return false;
  };

  using ObjectIdHelper =
    CIOCommon::IOObjectIdHelper<IO_CLASS_DUMMY_NOTIFY, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(SObjectId::EObjectDataType dtype,
                                                bool ts,
                                                uint16_t inst)
  {
    return ObjectIdHelper::create(dtype, ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInitval(uint8_t dtype, bool rw, uint8_t dim)
  {
    uint8_t words = dim;

    if (dtype == SObjectId::DTYPE_UINT64 || dtype == SObjectId::DTYPE_INT64 ||
        dtype == SObjectId::DTYPE_DOUBLE)
      {
        words = dim * 2;
      }

    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_DUMMY_NOTIFY,
                                 dtype,
                                 rw,
                                 words,
                                 IO_DUMMY_NOTIFY_CFG_INITVAL);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgInterval(bool rw)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_DUMMY_NOTIFY,
                                 SObjectId::DTYPE_UINT32,
                                 rw,
                                 1,
                                 IO_DUMMY_NOTIFY_CFG_INTERVAL);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdDim()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_DUMMY_NOTIFY,
                                 SObjectId::DTYPE_UINT32,
                                 false,
                                 1,
                                 IO_DUMMY_NOTIFY_CFG_DIM);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgNotifyOnWrite(bool rw = false)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_DUMMY_NOTIFY,
                                 SObjectId::DTYPE_UINT32,
                                 rw,
                                 1,
                                 IO_DUMMY_NOTIFY_CFG_NOTIFY_ON_WRITE);
  }

private:
  const uint32_t *cfgval; ///< Pointer to init-value words stored in descriptor config.
  void *val;              ///< Stored payload bytes.
  io_ddata_t *writeData;  ///< Init-allocated payload for write notifications.
  bool notifyOnWrite;     ///< Emit a notifier callback on successful writes.
  std::mutex mutex;       ///< Mutex protecting the backing value.
  size_t dlen;            ///< Data dimension.
  size_t cfglen;          ///< Number of configured init-value elements.
  size_t tlen;            ///< Element size in bytes.

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;            ///< Last timestamp associated with the stored value.
#endif

  int configureDesc(const CDescObject &desc);
  int setVal(const void *v, size_t d);
  int applyInitval();
};
} // Namespace dawn
