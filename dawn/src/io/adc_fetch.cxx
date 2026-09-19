// dawn/src/io/adc_fetch.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/adc_fetch.hxx"

using namespace dawn;

int CIOAdcFetch::getDataImpl(IODataCmn &data, size_t len)
{
  auto *dst = static_cast<uint8_t *>(data.getDataPtr());
  size_t expected = getDataSize();
  size_t total = 0;
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

  // Drain the whole conversion - a short read leaves samples in the FIFO
  // and rotates the channel order of the next fetch

  while (total < expected)
    {
      ret = adc_read(getFdInternal(),
                     reinterpret_cast<dawn::porting::adc_read_s *>(dst + total),
                     expected - total);
      if (ret < 0)
        {
          return ret;
        }

      if (ret == 0)
        {
          return -EIO;
        }

      total += static_cast<size_t>(ret);
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
