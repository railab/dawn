// dawn/include/dawn/io/pwm.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/porting/pwm.hxx"

namespace dawn
{
/**
 * @brief Pulse Width Modulation (PWM) output I/O type.
 *
 * Provides interface for writing PWM signals to PWM output devices.
 */

class CIOPwm : public CIOCommon
{
public:
  enum
  {
    IO_PWM_CFG_FIRST = 0, ///< First config ID (reserved).
    IO_PWM_CFG_FREQ = 1,  ///< PWM frequency configuration.
    IO_PWM_CFG_DT = 2,    ///< PWM dead time configuration.
    IO_PWM_CFG_POL = 3,   ///< PWM polarity configuration.
    IO_PWM_CFG_LAST = 31  ///< Last config ID marker (reserved).
  };

  explicit CIOPwm(CDescObject &desc)
    : CIOCommon(desc)
    , path()
    , fd(-1)
    , channels(CONFIG_PWM_NCHANNELS)
    , freq(CONFIG_DAWN_IO_PWM_DEFAULT_FREQ)
    , info(nullptr)
  {
  }

  ~CIOPwm() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "pwm";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int setDataImpl(IODataCmn &data) override;
  int doStart() override;
  int doStop() override;
  int trigger(uint8_t cmd) override;
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

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_PWM, SObjectId::DTYPE_UINT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_PWM, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdFreq(bool rw = false)
  {
    return CIOPwm::cfgId(rw, SObjectId::DTYPE_UINT32, 1, IO_PWM_CFG_FREQ);
  }

protected:
  int onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len) override;

private:
  char path[PATH_MAX];              ///< PWM device file path.
  int fd;                           ///< File descriptor for PWM device.
  uint8_t channels;                 ///< Number of PWM output channels.
  uint32_t freq;                    ///< PWM output frequency in Hz.
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  uint64_t ts;                      ///< Data update timestamp.
#endif
  dawn::porting::pwm_write_s *info; ///< PWM write data structure (platform-specific).

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
