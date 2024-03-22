// dawn/src/io/buttons.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/buttons.hxx"

#include "dawn/io/buttons.hxx"

using namespace dawn;

int CIOButtons::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY)
        {
          DAWNERR("unsupported buttons cfg 0x08%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      offset += cfgCmnOffset(item);
    }

  return OK;
}

CIOButtons::~CIOButtons()
{
  deinit();
}

int CIOButtons::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  // Get path to BUTTONS

  if (getCmnDevno() == -1)
    {
      DAWNERR("BUTTONS device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), BUTTONS_PATH_FMT, getCmnDevno());

  // Open file

  fd = buttons_open(path);
  if (fd < 0)
    {
      DAWNERR("buttons_open failed %d\n", -errno);
      return -errno;
    }

  return OK;
}

int CIOButtons::deinit()
{
  // Close file

  buttons_close(fd);
  return OK;
}

int CIOButtons::getDataImpl(IODataCmn &data, size_t len)
{
  uint32_t *tmp = static_cast<uint32_t *>(data.getDataPtr());
  uint32_t val;
  int ret;

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  // Read input

  ret = buttons_read(fd, &val);
  if (ret < 0)
    {
      DAWNERR("buttons_read failed %d\n", ret);
      return ret;
    }

  if (isTimestamp())
    {
      data.getTs() = getTimestamp();
    }

  *tmp = (uint32_t)val;

  return OK;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIOButtons::getFd() const
{
  return fd;
}
#endif

size_t CIOButtons::getDataSize() const
{
  return sizeof(uint32_t);
}

size_t CIOButtons::getDataDim() const
{
  return 1;
}
