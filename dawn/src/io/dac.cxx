// dawn/src/io/dac.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dac.hxx"

using namespace dawn;

int CIODac::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY)
        {
          DAWNERR("Unsupported DAC config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      offset += cfgCmnOffset(item);
    }

  return OK;
}

CIODac::~CIODac()
{
  deinit();
}

int CIODac::configure()
{
  int ret;

  if (getDtype() != SObjectId::DTYPE_INT32)
    {
      DAWNERR("DAC requires DTYPE_INT32\n");
      return -EINVAL;
    }

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("DAC configure failed (error %d)\n", ret);
      return ret;
    }

  // Get path to DAC

  if (getCmnDevno() == -1)
    {
      DAWNERR("DAC device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), DAC_PATH_FMT, getCmnDevno());

  // Open file

  fd = dac_open(path);
  if (fd < 0)
    {
      DAWNERR("failed to open file %d\n", -errno);
      return -errno;
    }

  // Intialize gpi

  dac_init(fd);

  return OK;
}

int CIODac::deinit()
{
  dac_close(fd);
  return OK;
}

int CIODac::setDataImpl(IODataCmn &data)
{
  int32_t *tmp = static_cast<int32_t *>(data.getDataPtr());
  dawn::porting::dac_write_s info;
  int ret;

  // Data dimmention must match DAC supported channels

  if (data.getItems() > 1)
    {
      return -ENOMEM;
    }

  // get data to write

  info.data = *tmp;

  // Write output

  ret = dac_write(fd, &info);
  if (ret < 0)
    {
      DAWNERR("dac_write failed %d\n", ret);
      return ret;
    }

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      ts = getTimestamp();
    }
#endif

  return OK;
}

size_t CIODac::getDataSize() const
{
  return sizeof(uint32_t) * channels;
}

size_t CIODac::getDataDim() const
{
  return channels;
}
