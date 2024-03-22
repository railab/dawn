// dawn/include/dawn/io/dac.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <mutex>

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/porting/dac.hxx"

namespace dawn
{
/**
 * @brief Digital-to-Analog Converter (DAC) output I/O type.
 *
 * Provides interface for writing analog output values to DAC devices.
 */

class CIODac : public CIOCommon
{
public:
  enum
  {
    IO_DAC_CFG_FIRST = 0,
    IO_DAC_CFG_LAST = 31
  };

  explicit CIODac(CDescObject &desc)
    : CIOCommon(desc)
    , path()
    , fd(-1)
    , channels(1)
  {
  }

  ~CIODac() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "dac";
  }
#endif

  int configure() override;
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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_DAC, SObjectId::DTYPE_INT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_DAC, dtype, rw, size, id);
  }

private:
  char path[PATH_MAX]; ///< DAC device file path.
  int fd;              ///< File descriptor for DAC device.
  uint8_t channels;    ///< Number of DAC output channels.
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;         ///< Data update timestamp.
#endif

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
