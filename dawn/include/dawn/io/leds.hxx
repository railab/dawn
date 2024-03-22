// dawn/include/dawn/io/leds.hxx
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
 * @brief LED output I/O type for controlling LED indicators.
 *
 * Provides interface for controlling LED states through LED output devices.
 */

class CIOLeds : public CIOCommon
{
public:
  enum
  {
    IO_LEDS_CFG_FIRST = 0,
    IO_LEDS_CFG_INITVAL = 1,
    IO_LEDS_CFG_LAST = 31
  };

  explicit CIOLeds(CDescObject &desc)
    : CIOCommon(desc)
    , fd(-1)
    , initVal(0)
  {
  }

  ~CIOLeds() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "led";
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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_LEDS, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_LEDS, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInitVal(uint32_t val = 0)
  {
    return CIOLeds::cfgId(false, SObjectId::DTYPE_UINT32, 1, IO_LEDS_CFG_INITVAL);
  }

private:
  char path[PATH_MAX] = {}; ///< LED device file path.
  int fd;                   ///< File descriptor for LED device.
  uint32_t initVal;         ///< Value written during init().
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;              ///< Data update timestamp.
#endif

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
