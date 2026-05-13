// dawn/src/io/rgbled.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/rgbled.hxx"

#include <cstdio>

using namespace dawn;

int CIORgbLed::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY &&
          item->cfgid.s.cls != CIOCommon::IO_CLASS_RGBLED)
        {
          DAWNERR("unsupported rgbled cfg 0x08%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      if (item->cfgid.s.cls == CIOCommon::IO_CLASS_ANY)
        {
          offset += cfgCmnOffset(item);
          continue;
        }

      switch (item->cfgid.s.id)
        {
          case IO_RGBLED_CFG_INITVAL:
            if (item->cfgid.s.size > 0)
              {
                initVal = *reinterpret_cast<const uint32_t *>(&item->data[0]);
              }
            offset += 1 + item->cfgid.s.size;
            break;

          default:
            DAWNERR("unsupported rgbled cfg id %u\n", item->cfgid.s.id);
            return -EINVAL;
        }
    }

  return OK;
}

CIORgbLed::~CIORgbLed()
{
  deinit();
}

int CIORgbLed::configure()
{
  int ret;

  if (getDtype() != SObjectId::DTYPE_UINT32)
    {
      DAWNERR("RGBLED requires DTYPE_UINT32\n");
      return -EINVAL;
    }

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  if (getCmnDevno() == -1)
    {
      DAWNERR("RGBLED device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), RGBLED_PATH_FMT, getCmnDevno());

  fd = rgbled_open(path);
  if (fd < 0)
    {
      DAWNERR("rgbled_open failed %d\n", fd);
      return fd;
    }

  return OK;
}

int CIORgbLed::init()
{
  if (fd < 0)
    {
      return -EIO;
    }

  return writeRgb(initVal);
}

int CIORgbLed::deinit()
{
  rgbled_close(fd);
  fd = -1;
  return OK;
}

int CIORgbLed::writeRgb(uint32_t val)
{
  char rgb[8];
  int ret;

  if (fd < 0)
    {
      return -EIO;
    }

  // TODO: add ioctl call in nuttx so we can write RAW bytes, not string

  snprintf(rgb,
           sizeof(rgb),
           "#%02X%02X%02X",
           static_cast<unsigned int>((val >> 16) & 0xffu),
           static_cast<unsigned int>((val >> 8) & 0xffu),
           static_cast<unsigned int>(val & 0xffu));

  ret = rgbled_write(fd, rgb, sizeof(rgb));
  if (ret < 0)
    {
      DAWNERR("rgbled_write failed %d\n", ret);
      return ret;
    }

  currentVal = val & 0x00ffffffu;

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      ts = getTimestamp();
    }
#endif

  return OK;
}

int CIORgbLed::getDataImpl(IODataCmn &data, size_t len)
{
  uint32_t *val;

  if (len != 1)
    {
      return -ENOTSUP;
    }

  val = static_cast<uint32_t *>(data.getDataPtr());
  *val = currentVal;

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      data.getTs() = ts;
    }
#endif

  return OK;
}

int CIORgbLed::setDataImpl(IODataCmn &data)
{
  uint32_t *val = static_cast<uint32_t *>(data.getDataPtr());

  return writeRgb(*val);
}

size_t CIORgbLed::getDataSize() const
{
  return sizeof(uint32_t);
}

size_t CIORgbLed::getDataDim() const
{
  return 1;
}
