// dawn/src/io/adc_fetch.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/adc_fetch.hxx"

using namespace dawn;

int CIOAdcFetch::getDataImpl(IODataCmn &data, size_t len)
{
  auto *ptr = reinterpret_cast<dawn::porting::adc_read_s *>(data.getDataPtr());
  int ret;

  if (len != 1)
    {
      return -ENOTSUP;
    }

  if (data.getItems() < getChans())
    {
      return -ENOMEM;
    }

  ret = adc_start(getFdInternal());
  if (ret < 0)
    {
      return ret;
    }

  ret = adc_read(getFdInternal(), ptr, data.getDataSize());
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      data.getTs() = getTimestamp();
    }
#endif

  return OK;
}

int CIOAdcFetch::doStart()
{
  return OK;
}

int CIOAdcFetch::doStop()
{
  return OK;
}

int CIOAdcFetch::trigger(uint8_t cmd)
{
  UNUSED(cmd);
  return adc_start(getFdInternal());
}
