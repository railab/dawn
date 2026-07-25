// dawn/src/io/pot.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/pot.hxx"

#include <cstdio>

using namespace dawn;

int CIOPot::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY &&
          item->cfgid.s.cls != CIOCommon::IO_CLASS_POT)
        {
          DAWNERR("Unsupported POT config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      if (item->cfgid.s.cls == CIOCommon::IO_CLASS_ANY)
        {
          offset += cfgCmnOffset(item);
          continue;
        }

      switch (item->cfgid.s.id)
        {
          case IO_POT_CFG_WIPER:
            if (item->cfgid.s.size > 0)
              {
                wiper = static_cast<uint8_t>(*reinterpret_cast<const uint32_t *>(&item->data[0]));
              }
            offset += 1 + item->cfgid.s.size;
            break;

          default:
            DAWNERR("unsupported POT cfg id %u\n", item->cfgid.s.id);
            return -EINVAL;
        }
    }

  return OK;
}

CIOPot::~CIOPot()
{
  deinit();
}

int CIOPot::configure()
{
  int ret;

  if (getDtype() != SObjectId::DTYPE_INT32)
    {
      DAWNERR("POT requires DTYPE_INT32\n");
      return -EINVAL;
    }

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("POT configure failed (error %d)\n", ret);
      return ret;
    }

  // Get path to POT

  if (getCmnDevno() == -1)
    {
      DAWNERR("POT device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), POT_PATH_FMT, getCmnDevno());

  // Open file

  fd = pot_open(path);
  if (fd < 0)
    {
      DAWNERR("failed to open file %d\n", -errno);
      return -errno;
    }

  return OK;
}

int CIOPot::deinit()
{
  pot_close(fd);
  return OK;
}

int CIOPot::setDataImpl(IODataCmn &data)
{
  int32_t *tmp = static_cast<int32_t *>(data.getDataPtr());
  dawn::porting::pot_rw_s info;
  int ret;

  // Only a single wiper value supported

  if (data.getItems() > 1)
    {
      return -ENOMEM;
    }

  // Get data to write

  info.wiper = wiper;
  info.val = static_cast<uint32_t>(*tmp);

  // Write wiper value

  ret = pot_set_wiper(fd, &info);
  if (ret < 0)
    {
      DAWNERR("pot_set_wiper failed %d\n", ret);
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

int CIOPot::getDataImpl(IODataCmn &data, size_t len)
{
  int32_t *tmp = static_cast<int32_t *>(data.getDataPtr());
  dawn::porting::pot_rw_s info;
  int ret;

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  // Read wiper value

  info.wiper = wiper;
  info.val = 0;

  ret = pot_get_wiper(fd, &info);
  if (ret < 0)
    {
      DAWNERR("pot_get_wiper failed %d\n", ret);
      return ret;
    }

  *tmp = static_cast<int32_t>(info.val);

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      data.getTs() = ts;
    }
#endif

  return OK;
}

size_t CIOPot::getDataSize() const
{
  return sizeof(int32_t);
}

size_t CIOPot::getDataDim() const
{
  return 1;
}
