// dawn/src/io/adc_stream.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/adc_stream.hxx"

#include <cinttypes>

using namespace dawn;

int CIOAdcStream::configureDesc(const CDescObject &desc)
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

          case CIOCommon::IO_CLASS_ADC_STREAM:
            {
              switch (item->cfgid.s.id)
                {
                  case IO_ADC_STREAM_CFG_BATCH_SIZE:
                    {
                      if (item->cfgid.s.size != 1)
                        {
                          DAWNERR("invalid adc_stream batch cfg size %d\n", item->cfgid.s.size);
                          return -EINVAL;
                        }

                      batchSize = *reinterpret_cast<const uint32_t *>(&item->data);
                      if (batchSize == 0)
                        {
                          DAWNERR("adc_stream batch size must be > 0\n");
                          return -EINVAL;
                        }

                      offset += 2;
                      break;
                    }

                  default:
                    {
                      DAWNERR("unsupported adc_stream cfg 0x%08" PRIx32 "\n", item->cfgid.v);
                      return -EINVAL;
                    }
                }
              break;
            }

          default:
            {
              DAWNERR("unsupported adc_stream cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CIOAdcStream::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("adc_stream configure failed (error %d)\n", ret);
      return ret;
    }

  ret = configureDevice();
  if (ret != OK)
    {
      return ret;
    }

  return ensureReadBuffer(batchSize);
}

int CIOAdcStream::getDataImpl(IODataCmn &data, size_t len)
{
  if (len == 0)
    {
      return -EINVAL;
    }

  if (len > batchSize)
    {
      return -EINVAL;
    }

  return readSamples(data, len);
}

int CIOAdcStream::doStart()
{
  return adc_start(getFdInternal());
}

int CIOAdcStream::doStop()
{
  return adc_stop(getFdInternal());
}

int CIOAdcStream::trigger(uint8_t cmd)
{
  UNUSED(cmd);
  return adc_start(getFdInternal());
}

int CIOAdcStream::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  int ret;

  if (SObjectCfg::objectCfgGetId(objcfg) != IO_ADC_STREAM_CFG_BATCH_SIZE)
    {
      return OK;
    }

  if (len != 1 || data[0] == 0)
    {
      return -EINVAL;
    }

  batchSize = data[0];
  ret = ensureReadBuffer(batchSize);
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}
