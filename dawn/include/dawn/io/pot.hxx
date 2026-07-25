// dawn/include/dawn/io/pot.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/porting/pot.hxx"

namespace dawn
{
/**
 * @brief Digital potentiometer (POT) wiper I/O type.
 *
 * Provides interface for a single wiper of a digital potentiometer
 * device. Writing sets the wiper value, reading returns the current
 * wiper value.
 */

class CIOPot : public CIOCommon
{
public:
  enum
  {
    IO_POT_CFG_FIRST = 0,
    IO_POT_CFG_WIPER = 1,
    IO_POT_CFG_LAST = 31
  };

  explicit CIOPot(CDescObject &desc)
    : CIOCommon(desc)
    , path()
    , fd(-1)
    , wiper(0)
  {
  }

  ~CIOPot() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "pot";
  }
#endif

  int configure() override;
  int deinit() override;
  int setDataImpl(IODataCmn &data) override;
  int getDataImpl(IODataCmn &data, size_t len) override;
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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_POT, SObjectId::DTYPE_INT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_POT, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdWiper(uint32_t val = 0)
  {
    return CIOPot::cfgId(false, SObjectId::DTYPE_UINT32, 1, IO_POT_CFG_WIPER);
  }

private:
  char path[PATH_MAX]; ///< POT device file path.
  int fd;              ///< File descriptor for POT device.
  uint8_t wiper;       ///< Wiper index.
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;         ///< Data update timestamp.
#endif

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
