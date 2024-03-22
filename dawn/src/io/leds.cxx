// dawn/src/io/leds.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/leds.hxx"

#include "dawn/io/leds.hxx"

using namespace dawn;

int CIOLeds::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY &&
          item->cfgid.s.cls != CIOCommon::IO_CLASS_LEDS)
        {
          DAWNERR("unsupported leds cfg 0x08%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      if (item->cfgid.s.cls == CIOCommon::IO_CLASS_ANY)
        {
          offset += cfgCmnOffset(item);
          continue;
        }

      switch (item->cfgid.s.id)
        {
          case IO_LEDS_CFG_INITVAL:
            if (item->cfgid.s.size > 0)
              {
                initVal = *reinterpret_cast<const uint32_t *>(&item->data[0]);
              }
            offset += 1 + item->cfgid.s.size;
            break;

          default:
            DAWNERR("unsupported leds cfg id %u\n", item->cfgid.s.id);
            return -EINVAL;
        }
    }

  return OK;
}

CIOLeds::~CIOLeds()
{
}

int CIOLeds::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  // Get path to LEDS

  if (getCmnDevno() == -1)
    {
      DAWNERR("LEDS device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), LEDS_PATH_FMT, getCmnDevno());

  // Open file

  fd = leds_open(path);
  if (fd < 0)
    {
      DAWNERR("leds_open failed %d\n", -errno);
      return -errno;
    }

  return OK;
}

int CIOLeds::init()
{
  if (fd < 0)
    {
      return -EIO;
    }

  uint32_t val = initVal;
  int ret = leds_write(fd, val);
  if (ret < 0)
    {
      DAWNERR("leds init write failed %d\n", ret);
      return ret;
    }

  return OK;
}

int CIOLeds::deinit()
{
  return OK;
}

int CIOLeds::getDataImpl(IODataCmn &data, size_t len)
{
  uint32_t *val;
  int ret;

  if (len != 1)
    {
      return -ENOTSUP;
    }

  val = static_cast<uint32_t *>(data.getDataPtr());
  ret = leds_read(fd, val);
  if (ret < 0)
    {
      DAWNERR("leds_read failed %d\n", ret);
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

int CIOLeds::setDataImpl(IODataCmn &data)
{
  uint32_t *val = static_cast<uint32_t *>(data.getDataPtr());
  int ret;

  // Write output

  ret = leds_write(fd, *val);
  if (ret < 0)
    {
      DAWNERR("leds_write failed %d\n", ret);
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

size_t CIOLeds::getDataSize() const
{
  return sizeof(uint32_t);
}

size_t CIOLeds::getDataDim() const
{
  return 1;
}
