// dawn/include/dawn/io/adc_fetch.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/adc_base.hxx"

namespace dawn
{
/** @brief On-demand ADC fetch I/O. */

class CIOAdcFetch : public CIOAdcBase
{
public:
  explicit CIOAdcFetch(CDescObject &desc)
    : CIOAdcBase(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "adc_fetch";
  }
#endif

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
    return false;
  }

  bool isBatch() const override
  {
    return false;
  }

  using ObjectIdHelper = CIOCommon::IOObjectIdHelper<IO_CLASS_ADC_FETCH, SObjectId::DTYPE_INT32>;

  constexpr static SObjectId::ObjectId objectId(bool ts, uint16_t inst)
  {
    return ObjectIdHelper::create(ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_IO, CIOCommon::IO_CLASS_ADC_FETCH, dtype, rw, size, id);
  }
};

} // Namespace dawn
