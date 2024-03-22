// dawn/src/io/gpi.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/gpi.hxx"

using namespace dawn;

int CIOGpi::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY)
        {
          DAWNERR("Unsupported GPI config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      offset += cfgCmnOffset(item);
    }

  return OK;
}

CIOGpi::~CIOGpi()
{
  deinit();
}

int CIOGpi::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("GPI configure failed (error %d)\n", ret);
      return ret;
    }

  // Get path to GPI

  if (getCmnDevno() == -1)
    {
      DAWNERR("GPI device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), GPI_PATH_FMT, getCmnDevno());

  // Open file

  fd = gpi_open(path);
  if (fd < 0)
    {
      DAWNERR("gpi_open failed %d\n", -errno);
      return -errno;
    }

  // Intialize gpi

  gpi_init(fd);

#ifdef CONFIG_DAWN_IO_NOTIFY
  // Register interrupt pin is required if notifications supported

  isNotifyIO = true;
  if (gpi_notify(fd) == false)
    {
      // Notify not supported for this GPI

      isNotifyIO = false;
    }
#endif

  return OK;
}

int CIOGpi::init()
{
  if (getDtype() != SObjectId::DTYPE_UINT32)
    {
      DAWNERR("GPI requires DTYPE_UINT32\n");
      return -EINVAL;
    }

  return OK;
}

int CIOGpi::deinit()
{
  // Close file

  gpi_close(fd);
  return OK;
}

int CIOGpi::getDataImpl(IODataCmn &data, size_t len)
{
  bool *tmp = static_cast<bool *>(data.getDataPtr());
  bool invalue;
  int ret;

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  // Read input

  ret = gpi_read(fd, &invalue);
  if (ret < 0)
    {
      DAWNERR("gpi_read failed %d\n", ret);
      return ret;
    }

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      data.getTs() = getTimestamp();
    }
#endif

  *tmp = static_cast<bool>(invalue);

  return OK;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIOGpi::getFd() const
{
  if (isNotifyIO)
    {
      return fd;
    }
  else
    {
      // Notify not supported

      return -1;
    }
}
#endif

size_t CIOGpi::getDataSize() const
{
  return sizeof(uint32_t);
}

size_t CIOGpi::getDataDim() const
{
  return 1;
}
