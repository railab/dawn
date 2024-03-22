// dawn/src/io/pwm.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/pwm.hxx"

#include <cstring>
#include <new>

using namespace dawn;

int CIOPwm::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
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

          case CIOCommon::IO_CLASS_PWM:
            {
              switch (item->cfgid.s.id)
                {
                  case IO_PWM_CFG_FREQ:
                    {
                      if (item->cfgid.s.size != 1)
                        {
                          DAWNERR("invalid PWM frequency config size %d\n", item->cfgid.s.size);
                          return -EINVAL;
                        }

                      freq = *reinterpret_cast<const uint32_t *>(&item->data);
                      if (freq == 0)
                        {
                          DAWNERR("PWM frequency must be > 0\n");
                          return -EINVAL;
                        }

                      offset += item->cfgid.s.size + 1;
                      break;
                    }

                  default:
                    {
                      DAWNERR("Unsupported PWM config 0x%08" PRIx32 "\n", item->cfgid.v);
                      return -EINVAL;
                    }
                }

              break;
            }

          default:
            {
              DAWNERR("Unsupported PWM config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOPwm::~CIOPwm()
{
  deinit();
}

int CIOPwm::configure()
{
  int ret;
  int i;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("PWM configure failed (error %d)\n", ret);
      return ret;
    }

  // Get path to PWM

  if (getCmnDevno() == -1)
    {
      DAWNERR("PWM device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), PWM_PATH_FMT, getCmnDevno());

  // Open file

  fd = pwm_open(path);
  if (fd < 0)
    {
      DAWNERR("failed to open file %d\n", -errno);
      return -errno;
    }

  // Intialize gpi

  pwm_init(fd);

  // Allocate channels info

  info = static_cast<dawn::porting::pwm_write_s *>(::operator new(
    sizeof(dawn::porting::pwm_write_s) + channels * sizeof(info->channels[0]), std::nothrow));
  if (info == nullptr)
    {
      DAWNERR("failed to allocate PWM channel info\n");
      return -ENOMEM;
    }

  std::memset(info, 0, sizeof(dawn::porting::pwm_write_s) + channels * sizeof(info->channels[0]));

  // Initialzie frequency for channels info which is constant

  info->freq = freq;
  for (i = 0; i < channels; i++)
    {
      info->channels[i].channel = i + 1;
    }

  return OK;
}

int CIOPwm::init()
{
  if (getDtype() != SObjectId::DTYPE_UINT32)
    {
      DAWNERR("PWM requires DTYPE_UINT32\n");
      return -EINVAL;
    }

  return OK;
}

int CIOPwm::deinit()
{
  ::operator delete(info);
  info = nullptr;

  if (fd >= 0)
    {
      pwm_close(fd);
      fd = -1;
    }

  return OK;
}

int CIOPwm::setDataImpl(IODataCmn &data)
{
  uint32_t *tmp = static_cast<uint32_t *>(data.getDataPtr());
  int ret;

  // Data dimmention must match PWM supported channels

  if (data.getItems() < getDataDim())
    {
      return -EINVAL;
    }

  for (int i = 0; i < channels; i++)
    {
      // Valid data range is 0 to 1000 (0% - 100%)

      if (tmp[i] > 1000)
        {
          return -EINVAL;
        }

      // channels start from 1
      info->channels[i].duty = tmp[i];
      info->channels[i].channel = i + 1;
    }

  // Write output

  ret = pwm_write(fd, info);
  if (ret < 0)
    {
      DAWNERR("pwm_write failed %d\n", ret);
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

int CIOPwm::doStart()
{
  return pwm_start(fd);
}

int CIOPwm::doStop()
{
  return pwm_stop(fd);
}

int CIOPwm::trigger(uint8_t cmd)
{
  DAWNERR("CIOPwm::trigger not implemented\n");
  return -ENOSYS;
}

int CIOPwm::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  uint32_t oldFreq;
  int ret;

  if (SObjectCfg::objectCfgGetId(objcfg) != IO_PWM_CFG_FREQ)
    {
      return OK;
    }

  if (len != 1 || data == nullptr || data[0] == 0)
    {
      return -EINVAL;
    }

  if (info == nullptr)
    {
      freq = data[0];
      return OK;
    }

  oldFreq = info->freq;
  info->freq = data[0];

  if (fd >= 0)
    {
      ret = pwm_write(fd, info);
      if (ret < 0)
        {
          info->freq = oldFreq;
          DAWNERR("pwm frequency update failed %d\n", ret);
          return ret;
        }
    }

  freq = data[0];
  return OK;
}

size_t CIOPwm::getDataSize() const
{
  return sizeof(uint32_t) * channels;
}

size_t CIOPwm::getDataDim() const
{
  return channels;
}
