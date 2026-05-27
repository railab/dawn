// dawn/src/io/pulsecount.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/pulsecount.hxx"

#include <cinttypes>
#include <cerrno>
#include <cstdio>

using namespace dawn;

namespace
{
/* Keep pulsecount frequency above 0 Hz: NSEC_PER_SEC / period must not
 * round down to zero in the NuttX driver.
 */
constexpr uint64_t PULSECOUNT_MAX_PERIOD_NS = 1000000000ull;
}

int CIOPulseCount::validateTimings(uint32_t high, uint32_t low) const
{
  uint64_t period;

  if (high == 0 || low == 0)
    {
      DAWNERR("pulsecount timings must be > 0\n");
      return -EINVAL;
    }

  period = static_cast<uint64_t>(high) + static_cast<uint64_t>(low);
  if (period > PULSECOUNT_MAX_PERIOD_NS)
    {
      DAWNERR("pulsecount period too large: %" PRIu64 " ns\n", period);
      return -EINVAL;
    }

  return OK;
}

int CIOPulseCount::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  int ret;

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

          case CIOCommon::IO_CLASS_PULSECOUNT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("invalid pulsecount config size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              switch (item->cfgid.s.id)
                {
                  case IO_PULSECOUNT_CFG_HIGH_NS:
                    {
                      highNs = *reinterpret_cast<const uint32_t *>(&item->data);
                      offset += item->cfgid.s.size + 1;
                      break;
                    }

                  case IO_PULSECOUNT_CFG_LOW_NS:
                    {
                      lowNs = *reinterpret_cast<const uint32_t *>(&item->data);
                      offset += item->cfgid.s.size + 1;
                      break;
                    }

                  default:
                    {
                      DAWNERR("Unsupported pulsecount config 0x%08" PRIx32 "\n", item->cfgid.v);
                      return -EINVAL;
                    }
                }

              break;
            }

          default:
            {
              DAWNERR("Unsupported pulsecount config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  ret = validateTimings(highNs, lowNs);
  if (ret < 0)
    {
      return ret;
    }

  return OK;
}

int CIOPulseCount::writeCurrentConfig(uint32_t count)
{
  dawn::porting::pulsecount_write_s info;

  info.high_ns = highNs;
  info.low_ns = lowNs;
  info.count = count;

  return pulsecount_write(fd, &info);
}

CIOPulseCount::~CIOPulseCount()
{
  deinit();
}

int CIOPulseCount::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("pulsecount configure failed (error %d)\n", ret);
      return ret;
    }

  if (getCmnDevno() == -1)
    {
      DAWNERR("pulsecount device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), PULSECOUNT_PATH_FMT, getCmnDevno());

  fd = pulsecount_open(path);
  if (fd < 0)
    {
      DAWNERR("failed to open file %d\n", -errno);
      return -errno;
    }

  pulsecount_init(fd);

  return OK;
}

int CIOPulseCount::init()
{
  if (getDtype() != SObjectId::DTYPE_UINT32)
    {
      DAWNERR("pulsecount requires DTYPE_UINT32\n");
      return -EINVAL;
    }

  return OK;
}

int CIOPulseCount::deinit()
{
  if (fd >= 0)
    {
      pulsecount_close(fd);
      fd = -1;
    }

  return OK;
}

int CIOPulseCount::setDataImpl(IODataCmn &data)
{
  uint32_t *tmp = static_cast<uint32_t *>(data.getDataPtr());
  int ret;

  if (data.getItems() < getDataDim())
    {
      return -EINVAL;
    }

  if (tmp[0] == 0)
    {
      return -EINVAL;
    }

  ret = writeCurrentConfig(tmp[0]);
  if (ret < 0)
    {
      DAWNERR("pulsecount_write failed %d\n", ret);
      return ret;
    }

  ret = pulsecount_start(fd);
  if (ret < 0)
    {
      DAWNERR("pulsecount_start failed %d\n", ret);
      return ret;
    }

  lastCount = tmp[0];

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      ts = getTimestamp();
    }
#endif

  return OK;
}

int CIOPulseCount::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  uint32_t newHigh;
  uint32_t newLow;
  int ret;

  if (len != 1 || data == nullptr)
    {
      return -EINVAL;
    }

  switch (SObjectCfg::objectCfgGetId(objcfg))
    {
      case IO_PULSECOUNT_CFG_HIGH_NS:
        newHigh = data[0];
        newLow = lowNs;
        break;

      case IO_PULSECOUNT_CFG_LOW_NS:
        newHigh = highNs;
        newLow = data[0];
        break;

      default:
        return OK;
    }

  ret = validateTimings(newHigh, newLow);
  if (ret < 0)
    {
      return ret;
    }

  if (lastCount != 0 && fd >= 0)
    {
      uint32_t oldHigh = highNs;
      uint32_t oldLow = lowNs;

      highNs = newHigh;
      lowNs = newLow;
      ret = writeCurrentConfig(lastCount);
      if (ret < 0)
        {
          highNs = oldHigh;
          lowNs = oldLow;
          return ret;
        }
    }

  highNs = newHigh;
  lowNs = newLow;
  return OK;
}

size_t CIOPulseCount::getDataSize() const
{
  return sizeof(uint32_t);
}

size_t CIOPulseCount::getDataDim() const
{
  return 1;
}
