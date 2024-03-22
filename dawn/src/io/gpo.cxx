// dawn/src/io/gpo.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/gpo.hxx"

using namespace dawn;

int CIOGpo::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY)
        {
          DAWNERR("Unsupported GPO config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      offset += cfgCmnOffset(item);
    }

  return OK;
}

CIOGpo::~CIOGpo()
{
  deinit();
}

int CIOGpo::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("GPO configure failed (error %d)\n", ret);
      return ret;
    }

  // Get path to GPO

  if (getCmnDevno() == -1)
    {
      DAWNERR("GPO device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), GPO_PATH_FMT, getCmnDevno());

  // Open file

  fd = gpo_open(path);
  if (fd < 0)
    {
      DAWNERR("gpo_open failed %d\n", -errno);
      return -errno;
    }

  // Intialize gpi

  gpo_init(fd);

  return OK;
}

int CIOGpo::init()
{
  if (getDtype() != SObjectId::DTYPE_UINT32)
    {
      DAWNERR("GPO requires DTYPE_UINT32\n");
      return -EINVAL;
    }

  return OK;
}

int CIOGpo::deinit()
{
  // Close file

  gpo_close(fd);
  return OK;
}

int CIOGpo::getDataImpl(IODataCmn &data, size_t len)
{
  bool *tmp = static_cast<bool *>(data.getDataPtr());
  bool invalue;
  int ret;

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  // Read output

  ret = gpo_read(fd, &invalue);
  if (ret < 0)
    {
      DAWNERR("gpi_read failed %d\n", ret);
      return ret;
    }

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      data.getTs() = ts;
    }
#endif

  *tmp = static_cast<bool>(invalue);

  return OK;
}

int CIOGpo::setDataImpl(IODataCmn &data)
{
  bool *outvalue = static_cast<bool *>(data.getDataPtr());
  int ret;

  // Write output

  ret = gpo_write(fd, *outvalue);
  if (ret < 0)
    {
      DAWNERR("gpo_write failed %d\n", ret);
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

size_t CIOGpo::getDataSize() const
{
  return sizeof(uint32_t);
}

size_t CIOGpo::getDataDim() const
{
  return 1;
}
