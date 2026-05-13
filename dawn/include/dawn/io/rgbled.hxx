// dawn/include/dawn/io/rgbled.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/porting/rgbled.hxx"

namespace dawn
{
/**
 * @brief RGB LED output I/O type.
 *
 * Controls RGB LED devices using a Dawn uint32_t value encoded as
 * 0x00RRGGBB. The IO converts the value to #RRGGBB string format
 * expected by the RGB LED character driver.
 */

class CIORgbLed : public CIOCommon
{
public:
  enum
  {
    IO_RGBLED_CFG_FIRST = 0,
    IO_RGBLED_CFG_INITVAL = 1,
    IO_RGBLED_CFG_LAST = 31
  };

  explicit CIORgbLed(CDescObject &desc)
    : CIOCommon(desc)
    , path()
    , fd(-1)
    , initVal(0)
    , currentVal(0)
#ifdef CONFIG_DAWN_IO_TIMESTAMP
    , ts(0)
#endif
  {
  }

  ~CIORgbLed() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "rgbled";
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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_RGBLED, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_RGBLED, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdInitVal(uint32_t val = 0)
  {
    return CIORgbLed::cfgId(false, SObjectId::DTYPE_UINT32, 1, IO_RGBLED_CFG_INITVAL);
  }

private:
  char path[PATH_MAX]; ///< RGB LED device file path.
  int fd;              ///< File descriptor for RGB LED device.
  uint32_t initVal;    ///< Value written during init().
  uint32_t currentVal; ///< Last value written by Dawn.
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;         ///< Data update timestamp.
#endif

  int configureDesc(const CDescObject &desc);
  int writeRgb(uint32_t val);
};
} // Namespace dawn
