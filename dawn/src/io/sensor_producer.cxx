// dawn/src/io/sensor_producer.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sensor_producer.hxx"

#include <cstdio>
#include <cstring>

#include <nuttx/sensors/sensor.h>

using namespace dawn;

int CIOSensorProducer::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      switch (item->cfgid.s.cls)
        {
          case CIOCommon::IO_CLASS_SENSOR_PRODUCER_CONFIG:
            {
              switch (item->cfgid.s.id)
                {
                  case CIOSensorProducer::IO_SENSOR_PRODUCER_CFG_QUEUE_SIZE:
                    {
                      const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);
                      queueSize = *tmp;
                      offset += 2;
                      break;
                    }

                  case CIOSensorProducer::IO_SENSOR_PRODUCER_CFG_PERSIST:
                    {
                      const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);
                      persist = (*tmp != 0);
                      offset += 2;
                      break;
                    }

                  default:
                    {
                      DAWNERR("Unsupported sensor producer config 0x%08" PRIx32 "\n",
                              item->cfgid.v);
                      return -EINVAL;
                    }
                }
              break;
            }

          case CIOCommon::IO_CLASS_ANY:
            {
              offset += cfgCmnOffset(item);
              break;
            }

          default:
            {
              DAWNERR("Unsupported sensor producer config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOSensorProducer::~CIOSensorProducer()
{
  deinit();
}

int CIOSensorProducer::configure()
{
  int dtypeSize;
  int ret;

  if (info == nullptr)
    {
      DAWNERR("Unsupported sensor producer class %d\n", getCls());
      return -EINVAL;
    }

#ifdef CONFIG_SENSORS_USE_B16
  if (getDtype() != SObjectId::DTYPE_B16)
    {
      DAWNERR("Sensor producer requires DTYPE_B16 when CONFIG_SENSORS_USE_B16 is enabled\n");
      return -EINVAL;
    }
#else
  if (getDtype() != SObjectId::DTYPE_FLOAT)
    {
      DAWNERR("Sensor producer requires DTYPE_FLOAT when CONFIG_SENSORS_USE_FLOAT is enabled\n");
      return -EINVAL;
    }
#endif

  dtypeSize = SObjectId::getDtypeSize_(static_cast<SObjectId::EObjectDataType>(getDtype()));
  if (dtypeSize != sizeof(sensor_data_t))
    {
      DAWNERR("Sensor producer dtype size %d does not match NuttX sensor data size %zu\n",
              dtypeSize,
              sizeof(sensor_data_t));
      return -EINVAL;
    }

  if (info->path == nullptr || info->rsize == 0 || info->dsize == 0 ||
      info->payloadOffset + getDataSize() > info->rsize)
    {
      DAWNERR("Unsupported sensor producer class %d\n", getCls());
      return -EINVAL;
    }

  if (DATA_BUFFER_SIZE < info->rsize)
    {
      DAWNERR(
        "Sensor producer event size %zu exceeds buffer size %zu\n", info->rsize, DATA_BUFFER_SIZE);
      return -EINVAL;
    }

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Sensor producer configure failed (error %d)\n", ret);
      return ret;
    }

  if (getCmnDevno() == -1)
    {
      DAWNERR("SENSOR producer device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), SENSOR_PATH_FMT, info->path, "", getCmnDevno());

  ret = sensor_user_register(path, info->rsize, queueSize, persist);
  if (ret == OK)
    {
      registered = true;
    }
  else if (ret != -EEXIST)
    {
      DAWNERR("failed to register sensor producer %s %d\n", path, ret);
      return ret;
    }

  fd = sensor_open_write(path);
  if (fd < 0)
    {
      if (registered)
        {
          sensor_user_unregister(path);
          registered = false;
        }
      return fd;
    }

  return OK;
}

int CIOSensorProducer::deinit()
{
  if (fd >= 0)
    {
      sensor_close(fd);
      fd = -1;
    }

  if (registered)
    {
      sensor_user_unregister(path);
      registered = false;
    }

  return OK;
}

int CIOSensorProducer::setDataImpl(IODataCmn &data)
{
  uint8_t buf[DATA_BUFFER_SIZE];
  io_ts_t *eventTs;
  int ret;

  if (fd < 0)
    {
      return -ENODEV;
    }

  if (data.getDataSize() != getDataSize())
    {
      return -EINVAL;
    }

  std::memset(buf, 0, info->rsize);
  eventTs = reinterpret_cast<io_ts_t *>(buf);
  *eventTs = sensor_get_timestamp();
  std::memcpy(buf + info->payloadOffset, data.getDataPtr(), data.getDataSize());

  ret = sensor_write(fd, buf, info->rsize);
  if (ret < 0)
    {
      DAWNERR("sensor producer write failed %d\n", ret);
      return ret;
    }

  if (static_cast<size_t>(ret) != info->rsize)
    {
      return -EIO;
    }

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      eventTs = reinterpret_cast<io_ts_t *>(buf);
      this->ts = *eventTs;
    }
#endif

  return OK;
}

size_t CIOSensorProducer::getDataSize() const
{
  return info != nullptr ? info->dsize * sizeof(sensor_data_t) : 0;
}

size_t CIOSensorProducer::getDataDim() const
{
  return getDataSize() / sizeof(sensor_data_t);
}
