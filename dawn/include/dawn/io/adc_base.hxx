// dawn/include/dawn/io/adc_base.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <limits.h>

#include "dawn/io/common.hxx"
#include "dawn/porting/adc.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/** @brief Shared ADC base for fetch/sync/stream implementations. */

class CIOAdcBase : public CIOCommon
{
public:
  enum
  {
    IO_ADC_CFG_FIRST = 0,
    IO_ADC_CFG_CHANLIST = 1,
    IO_ADC_CFG_LAST = 31
  };

  explicit CIOAdcBase(CDescObject &desc)
    : CIOCommon(desc)
    , rdata(nullptr)
    , fd(-1)
    , tlen(getDtypeSize())
    , chans(0)
    , batch(0)
  {
  }

  ~CIOAdcBase() override;

  int configure() override;
  int init() override;
  int deinit() override;

  int getFd() const override
  {
    return fd;
  }

  size_t getDataSize() const override;
  size_t getDataDim() const override;

protected:
  int configureCommonDesc(const CDescObject &desc);
  int configureDevice();
  int ensureReadBuffer(size_t samples);
  int readSamples(IODataCmn &data, size_t len);

  int getFdInternal() const
  {
    return fd;
  }

  size_t getChans() const
  {
    return chans;
  }

  size_t getTlen() const
  {
    return tlen;
  }

  dawn::porting::adc_read_s *getRdata() const
  {
    return rdata;
  }

private:
  dawn::porting::adc_read_s *rdata;
  char path[PATH_MAX] = {};
  int fd;
  size_t tlen;
  uint8_t chans;
  size_t batch;
};

} // Namespace dawn
