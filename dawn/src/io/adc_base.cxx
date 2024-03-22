// dawn/src/io/adc_base.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/adc_base.hxx"

#include <cerrno>
#include <cinttypes>
#include <cstdio>
#include <cstring>
#include <new>

using namespace dawn;

CIOAdcBase::~CIOAdcBase()
{
  deinit();
}

int CIOAdcBase::configureCommonDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY)
        {
          DAWNERR("Unsupported ADC config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      offset += cfgCmnOffset(item);
    }

  return OK;
}

int CIOAdcBase::configureDevice()
{
  int ret;

  if (getCmnDevno() == -1)
    {
      DAWNERR("ADC device number not configured\n");
      return -EINVAL;
    }

  std::snprintf(path, sizeof(path), ADC_PATH_FMT, getCmnDevno());

  fd = adc_open(path);
  if (fd < 0)
    {
      DAWNERR("failed to open ADC device %d\n", -fd);
      return fd;
    }

  ret = adc_get_nchans(fd);
  if (ret <= 0)
    {
      adc_close(fd);
      fd = -1;
      DAWNERR("invalid number of channels %d\n", ret);
      return ret < 0 ? ret : -EIO;
    }

  chans = static_cast<uint8_t>(ret);
  return OK;
}

int CIOAdcBase::configure()
{
  int ret;

  ret = configureCommonDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("ADC configure failed (error %d)\n", ret);
      return ret;
    }

  return configureDevice();
}

int CIOAdcBase::init()
{
  if (getDtype() != SObjectId::DTYPE_INT32)
    {
      DAWNERR("ADC requires DTYPE_INT32\n");
      return -EINVAL;
    }

  return OK;
}

int CIOAdcBase::deinit()
{
  delete[] rdata;
  rdata = nullptr;
  batch = 0;

  adc_close(fd);
  fd = -1;
  chans = 0;

  return OK;
}

size_t CIOAdcBase::getDataSize() const
{
  return chans * tlen;
}

size_t CIOAdcBase::getDataDim() const
{
  return chans;
}

int CIOAdcBase::ensureReadBuffer(size_t samples)
{
  dawn::porting::adc_read_s *tmp;

  if (samples == 0)
    {
      return -EINVAL;
    }

  if (chans == 0)
    {
      return -EACCES;
    }

  if (rdata != nullptr && batch >= samples)
    {
      return OK;
    }

  tmp = new (std::nothrow) dawn::porting::adc_read_s[samples * chans]();
  if (tmp == nullptr)
    {
      return -ENOMEM;
    }

  delete[] rdata;
  rdata = tmp;
  batch = samples;

  return OK;
}

int CIOAdcBase::readSamples(IODataCmn &data, size_t len)
{
  size_t bytesPerBatch;
  size_t expected;
  size_t total;
  uint8_t *dst;
  int ret;

  if (len == 0)
    {
      return -EINVAL;
    }

  if (data.getItems() < chans)
    {
      return -ENOMEM;
    }

  bytesPerBatch = getDataSize();
  expected = len * bytesPerBatch;
  total = 0;

  if (!data.hasTimestamp())
    {
      dst = static_cast<uint8_t *>(data.getDataPtr());
    }
  else
    {
      ret = ensureReadBuffer(len);
      if (ret < 0)
        {
          return ret;
        }

      dst = reinterpret_cast<uint8_t *>(rdata);
    }

  while (total < expected)
    {
      ret =
        adc_read(fd, reinterpret_cast<dawn::porting::adc_read_s *>(dst + total), expected - total);
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

  if (data.hasTimestamp())
    {
      for (size_t i = 0; i < len; i++)
        {
          std::memcpy(data.getDataPtr(i), &rdata[i * chans], bytesPerBatch);

#ifdef CONFIG_DAWN_IO_TIMESTAMP
          data.getTs(i) = getTimestamp();
#endif
        }
    }
#ifdef CONFIG_DAWN_IO_TIMESTAMP
  else if (isTimestamp())
    {
      for (size_t i = 0; i < len; i++)
        {
          data.getTs(i) = getTimestamp();
        }
    }
#endif

  return OK;
}
