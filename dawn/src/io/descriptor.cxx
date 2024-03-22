// dawn/src/io/descriptor.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/descriptor.hxx"

using namespace dawn;

int CIODescriptor::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  size_t i = 0;

  for (i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      switch (item->cfgid.s.cls)
        {
          case CIOCommon::IO_CLASS_ANY:
            {
              offset += cfgCmnOffset(item);
              break;
            }

          default:
            {
              DAWNERR("unsupported descriptor cfg 0x08%" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIODescriptor::~CIODescriptor()
{
  deinit();
}

int CIODescriptor::configure()
{
  CDevDescriptor *dev = CDevDescriptor::getInst();
  int ret;

  // Configure

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  // get descriptor data

  ret = dev->getDescriptor(getCmnDevno(), regDesc);
  if (ret < 0)
    {
      DAWNERR("Failed to get descriptor data\n");
    }

  return ret;
}

int CIODescriptor::deinit()
{
  return OK;
}

int CIODescriptor::getDataImpl(IODataCmn &data, size_t len)
{
  if (len != 1)
    {
      return -EINVAL;
    }

  if (data.getDataSize() < regDesc.len)
    {
      return -ENOMEM;
    }

  std::memcpy(data.getDataPtr(0), regDesc.ptr, regDesc.len);

  return OK;
}

int CIODescriptor::setDataImpl(IODataCmn &data)
{
  (void)data;
  return -ENOTSUP;
}

int CIODescriptor::setDataAtImpl(IODataCmn &data, size_t offset)
{
  CDevDescriptor *dev = CDevDescriptor::getInst();
  int ret;

  if (getCmnDevno() == 0)
    {
      return -EROFS;
    }

  ret = dev->writeSlotData(getCmnDevno(), data.getDataPtr(0), offset, data.getDataSize());
  if (ret < 0)
    {
      return ret;
    }

  ret = dev->getDescriptor(getCmnDevno(), regDesc);
  if (ret < 0)
    {
      DAWNERR("Failed to refresh descriptor data\n");
      return ret;
    }

  return OK;
}

int CIODescriptor::getDataAtImpl(IODataCmn &data, size_t len, size_t offset)
{
  size_t avail;
  size_t to_copy;

  if (len != 1)
    {
      return -EINVAL;
    }

  if (offset >= regDesc.len)
    {
      return -EINVAL;
    }

  avail = regDesc.len - offset;
  to_copy = data.getDataSize() < avail ? data.getDataSize() : avail;

  std::memcpy(data.getDataPtr(0), static_cast<const uint8_t *>(regDesc.ptr) + offset, to_copy);

  return OK;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIODescriptor::getFd() const
{
  // Not supported

  return -1;
};
#endif

size_t CIODescriptor::getDataSize() const
{
  return regDesc.len;
}

size_t CIODescriptor::getDataDim() const
{
  // Block payload uses 1-byte data units.
  return regDesc.len;
}
