// dawn/include/dawn/io/dummy.hxx
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
 * @brief Dummy I/O type for testing and simulation.
 *
 * Provides a fully functional memory-based I/O that supports both read and
 * write operations.
 */

class CIODummy : public CIOCommon
{
public:
  enum
  {
    IO_DUMMY_CFG_FIRST = 0,
    IO_DUMMY_CFG_INITVAL = 1,
    IO_DUMMY_CFG_DIM = 2,
    IO_DUMMY_CFG_LAST = 31
  };

  static_assert(IO_DUMMY_CFG_LAST - 1 <= SObjectCfg::ID_MAX);

  explicit CIODummy(CDescObject &desc)
    : CIOCommon(desc)
    , cfgval(nullptr)
    , val(nullptr)
    , dlen(1)
    , cfglen(1)
    , tlen(getDtypeSize())
#ifdef CONFIG_DAWN_IO_TIMESTAMP
    , ts(0)
#endif
  {
  }

  ~CIODummy() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "dummy";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;

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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_DUMMY, SObjectId::DTYPE_UINT32>;

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

    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, (CIOCommon::IO_CLASS_DUMMY), dtype, rw, words, IO_DUMMY_CFG_INITVAL);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdDim()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_DUMMY,
                                 SObjectId::DTYPE_UINT32,
                                 false,
                                 1,
                                 IO_DUMMY_CFG_DIM);
  }

private:
  const uint32_t *cfgval; ///< Pointer to init-value words stored in descriptor config.
  void *val;              ///< Allocated data storage buffer.
  std::mutex mutex;       ///< Mutex protecting data access.
  size_t dlen;            ///< Data size in bytes.
  size_t cfglen;          ///< Number of configured init-value elements.
  size_t tlen;            ///< Type size in bytes.

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;            ///< Timestamp of last data update.
#endif

  int setVal(const void *v, size_t d);
  int applyInitval();
  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
