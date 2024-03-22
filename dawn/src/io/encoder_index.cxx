// dawn/src/io/encoder_index.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/encoder_index.hxx"

#include <cerrno>
#include <cinttypes>
#include <cstdio>

using namespace dawn;

int CIOEncoderIndex::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      switch (item->cfgid.s.cls)
        {
          case CIOCommon::IO_CLASS_ANY:
            {
              offset += cfgCmnOffset(item);
              break;
            }

          case CIOCommon::IO_CLASS_ENCODER_INDEX:
            {
              switch (item->cfgid.s.id)
                {
                  case IO_ENCODER_INDEX_CFG_POSMAX:
                    {
                      if (item->cfgid.s.size != 1)
                        {
                          DAWNERR("invalid encoder_index posmax cfg size %d\n", item->cfgid.s.size);
                          return -EINVAL;
                        }

                      const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);
                      posmax = *tmp;
                      hasPosmax = true;
                      offset += 2;
                      break;
                    }

                  default:
                    {
                      DAWNERR("unsupported encoder_index cfg 0x%08" PRIx32 "\n", item->cfgid.v);
                      return -EINVAL;
                    }
                }
              break;
            }

          default:
            {
              DAWNERR("unsupported encoder_index cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOEncoderIndex::~CIOEncoderIndex()
{
  deinit();
}

int CIOEncoderIndex::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("encoder_index configure failed (error %d)\n", ret);
      return ret;
    }

  if (getCmnDevno() == -1)
    {
      DAWNERR("encoder_index device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), ENCODER_PATH_FMT, getCmnDevno());

  fd = encoder_open(path);
  if (fd < 0)
    {
      DAWNERR("failed to open encoder_index %d\n", -errno);
      return -errno;
    }

  if (hasPosmax)
    {
      ret = encoder_set_posmax(fd, posmax);
      if (ret < 0)
        {
          DAWNERR("encoder_index set posmax failed %d\n", ret);
          encoder_close(fd);
          fd = -1;
          return ret;
        }
    }

  return OK;
}

int CIOEncoderIndex::init()
{
  if (getDtype() != SObjectId::DTYPE_INT32)
    {
      DAWNERR("encoder_index requires DTYPE_INT32\n");
      return -EINVAL;
    }

  return OK;
}

int CIOEncoderIndex::deinit()
{
  encoder_close(fd);
  fd = -1;
  return OK;
}

int CIOEncoderIndex::getDataImpl(IODataCmn &data, size_t len)
{
  dawn::porting::encoder_index_s idx;
  int32_t *tmp = static_cast<int32_t *>(data.getDataPtr());
  int ret;

  if (len != 1)
    {
      return -ENOTSUP;
    }

  if (data.getItems() < getDataDim())
    {
      return -ENOMEM;
    }

  ret = encoder_read_index(fd, &idx);
  if (ret < 0)
    {
      DAWNERR("encoder_read_index failed %d\n", ret);
      return ret;
    }

  tmp[0] = idx.qenc_pos;
  tmp[1] = idx.indx_pos;
  tmp[2] = idx.indx_cnt;

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      data.getTs() = getTimestamp();
    }
#endif

  return OK;
}

int CIOEncoderIndex::trigger(uint8_t cmd)
{
  if (cmd == CObject::CMD_RESET)
    {
      return encoder_reset(fd);
    }

  return -ENOTSUP;
}
