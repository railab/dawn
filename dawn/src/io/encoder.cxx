// dawn/src/io/encoder.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/encoder.hxx"

#include <cerrno>
#include <cinttypes>
#include <cstdio>

using namespace dawn;

int CIOEncoder::configureDesc(const CDescObject &desc)
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

          case CIOCommon::IO_CLASS_ENCODER:
            {
              switch (item->cfgid.s.id)
                {
                  case IO_ENCODER_CFG_POSMAX:
                    {
                      if (item->cfgid.s.size != 1)
                        {
                          DAWNERR("invalid encoder posmax cfg size %d\n", item->cfgid.s.size);
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
                      DAWNERR("unsupported encoder cfg 0x%08" PRIx32 "\n", item->cfgid.v);
                      return -EINVAL;
                    }
                }
              break;
            }

          default:
            {
              DAWNERR("unsupported encoder cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOEncoder::~CIOEncoder()
{
  deinit();
}

int CIOEncoder::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("encoder configure failed (error %d)\n", ret);
      return ret;
    }

  if (getCmnDevno() == -1)
    {
      DAWNERR("encoder device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), ENCODER_PATH_FMT, getCmnDevno());

  fd = encoder_open(path);
  if (fd < 0)
    {
      DAWNERR("failed to open encoder %d\n", -errno);
      return -errno;
    }

  if (hasPosmax)
    {
      ret = encoder_set_posmax(fd, posmax);
      if (ret < 0)
        {
          DAWNERR("encoder set posmax failed %d\n", ret);
          encoder_close(fd);
          fd = -1;
          return ret;
        }
    }

  return OK;
}

int CIOEncoder::init()
{
  if (getDtype() != SObjectId::DTYPE_INT32)
    {
      DAWNERR("encoder requires DTYPE_INT32\n");
      return -EINVAL;
    }

  return OK;
}

int CIOEncoder::deinit()
{
  encoder_close(fd);
  fd = -1;
  return OK;
}

int CIOEncoder::getDataImpl(IODataCmn &data, size_t len)
{
  int32_t *tmp = static_cast<int32_t *>(data.getDataPtr());
  int ret;

  if (len != 1)
    {
      return -ENOTSUP;
    }

  ret = encoder_read_position(fd, tmp);
  if (ret < 0)
    {
      DAWNERR("encoder_read_position failed %d\n", ret);
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

int CIOEncoder::trigger(uint8_t cmd)
{
  if (cmd == CObject::CMD_RESET)
    {
      return encoder_reset(fd);
    }

  return -ENOTSUP;
}
