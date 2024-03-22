// dawn/include/dawn/io/adc_stream.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/adc_base.hxx"

namespace dawn
{
/** @brief High-throughput batched ADC stream I/O. */

class CIOAdcStream : public CIOAdcBase
{
public:
  enum
  {
    IO_ADC_STREAM_CFG_FIRST = 0,
    IO_ADC_STREAM_CFG_BATCH_SIZE = 1,
    IO_ADC_STREAM_CFG_LAST = 31
  };

  explicit CIOAdcStream(CDescObject &desc)
    : CIOAdcBase(desc)
    , batchSize(1)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "adc_stream";
  }
#endif

  int configure() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int doStart() override;
  int doStop() override;
  int trigger(uint8_t cmd) override;

  bool isRead() const override
  {
    return true;
  }

  bool isWrite() const override
  {
    return false;
  }

  bool isNotify() const override
  {
    return true;
  }

  bool isBatch() const override
  {
    return true;
  }

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_ADC_STREAM, SObjectId::DTYPE_INT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_ADC_STREAM, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdBatchSize(bool rw = false)
  {
    return CIOAdcStream::cfgId(rw, SObjectId::DTYPE_UINT32, 1, IO_ADC_STREAM_CFG_BATCH_SIZE);
  }

protected:
  int onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len) override;

private:
  size_t batchSize;

  int configureDesc(const CDescObject &desc);
};

} // Namespace dawn
