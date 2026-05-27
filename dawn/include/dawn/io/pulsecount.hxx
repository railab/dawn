// dawn/include/dawn/io/pulsecount.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/porting/pulsecount.hxx"

namespace dawn
{
/**
 * @brief Finite pulse-train output backed by the NuttX PULSECOUNT driver.
 *
 * Data writes provide the number of pulses to emit. Descriptor config selects
 * the high and low pulse durations.
 */

class CIOPulseCount : public CIOCommon
{
public:
  enum
  {
    IO_PULSECOUNT_CFG_FIRST = 0,   ///< First config ID (reserved).
    IO_PULSECOUNT_CFG_HIGH_NS = 1, ///< Pulse high time in nanoseconds.
    IO_PULSECOUNT_CFG_LOW_NS = 2,  ///< Pulse low time in nanoseconds.
    IO_PULSECOUNT_CFG_LAST = 31    ///< Last config ID marker (reserved).
  };

  explicit CIOPulseCount(CDescObject &desc)
    : CIOCommon(desc)
    , path()
    , fd(-1)
    , highNs(CONFIG_DAWN_IO_PULSECOUNT_DEFAULT_HIGH_NS)
    , lowNs(CONFIG_DAWN_IO_PULSECOUNT_DEFAULT_LOW_NS)
    , lastCount(0)
  {
  }

  ~CIOPulseCount() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "pulsecount";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_PULSECOUNT, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_PULSECOUNT, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdHighNs(bool rw = false)
  {
    return CIOPulseCount::cfgId(rw, SObjectId::DTYPE_UINT32, 1, IO_PULSECOUNT_CFG_HIGH_NS);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdLowNs(bool rw = false)
  {
    return CIOPulseCount::cfgId(rw, SObjectId::DTYPE_UINT32, 1, IO_PULSECOUNT_CFG_LOW_NS);
  }

protected:
  int onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len) override;

private:
  char path[PATH_MAX]; ///< Pulsecount device file path.
  int fd;              ///< File descriptor for pulsecount device.
  uint32_t highNs;     ///< Pulse high time in nanoseconds.
  uint32_t lowNs;      ///< Pulse low time in nanoseconds.
  uint32_t lastCount;  ///< Most recently configured pulse count.
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;         ///< Data update timestamp.
#endif

  int configureDesc(const CDescObject &desc);
  int validateTimings(uint32_t high, uint32_t low) const;
  int writeCurrentConfig(uint32_t count);
};
} // Namespace dawn
