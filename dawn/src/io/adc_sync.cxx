// dawn/src/io/adc_sync.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/adc_sync.hxx"

#include <cinttypes>

using namespace dawn;

int CIOAdcSync::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      switch (item->cfgid.s.cls)
        {
          case CIOCommon::IO_CLASS_ANY:
            {
              offset += cfgCmnOffset(item);
              break;
            }

          case CIOCommon::IO_CLASS_ADC_SYNC:
            {
              switch (item->cfgid.s.id)
                {
                  case IO_ADC_SYNC_CFG_TRIGGER_FREQ:
                    {
                      if (item->cfgid.s.size != 1)
                        {
                          DAWNERR("invalid adc_sync trigger freq cfg size %d\n",
                                  item->cfgid.s.size);
                          return -EINVAL;
                        }

                      triggerFreqHz = *reinterpret_cast<const uint32_t *>(&item->data);
                      if (triggerFreqHz == 0)
                        {
                          DAWNERR("adc_sync trigger frequency must be > 0\n");
                          return -EINVAL;
                        }

                      hasTriggerFreq = true;
                      offset += 2;
                      break;
                    }

                  default:
                    {
                      DAWNERR("unsupported adc_sync cfg 0x%08" PRIx32 "\n", item->cfgid.v);
                      return -EINVAL;
                    }
                }
              break;
            }

          default:
            {
              DAWNERR("unsupported adc_sync cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CIOAdcSync::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("adc_sync configure failed (error %d)\n", ret);
      return ret;
    }

  ret = configureDevice();
  if (ret != OK)
    {
      return ret;
    }

  if (hasTriggerFreq)
    {
      ret = adc_set_timer_freq(getFdInternal(), triggerFreqHz);
      if (ret < 0)
        {
          return ret;
        }
    }

  return OK;
}

int CIOAdcSync::getDataImpl(IODataCmn &data, size_t len)
{
  if (len == 0)
    {
      return -EINVAL;
    }

  return readSamples(data, len);
}

int CIOAdcSync::doStart()
{
  return OK;
}

int CIOAdcSync::doStop()
{
  return adc_stop(getFdInternal());
}

int CIOAdcSync::trigger(uint8_t cmd)
{
  UNUSED(cmd);
  return adc_start(getFdInternal());
}

int CIOAdcSync::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  int ret;

  if (SObjectCfg::objectCfgGetId(objcfg) != IO_ADC_SYNC_CFG_TRIGGER_FREQ)
    {
      return OK;
    }

  if (len != 1 || data[0] == 0)
    {
      return -EINVAL;
    }

  ret = adc_set_timer_freq(getFdInternal(), data[0]);
  if (ret < 0)
    {
      return ret;
    }

  triggerFreqHz = data[0];
  hasTriggerFreq = true;
  return OK;
}
