// dawn/src/io/sensor.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sensor.hxx"

#include <cstdio>
#include <cstring>

using namespace dawn;

int CIOSensor::configureDesc(const CDescObject &desc)
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
              switch (item->cfgid.s.id)
                {
                  case CIOSensor::IO_SENSOR_CFG_UPDATEINTERVAL:
                    {
                      const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);

                      updateInterval = *tmp;
                      offset += 2;
                      break;
                    }

                  case CIOSensor::IO_SENSOR_CFG_MEASPERIOD:
                    {
                      const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);

                      measurementPeriod = *tmp;
                      offset += 2;
                      break;
                    }

                  default:
                    {
                      offset += cfgCmnOffset(item);
                      break;
                    }
                }
              break;
            }

          default:
            {
              DAWNERR("Unsupported sensor config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOSensor::~CIOSensor()
{
  deinit();
}

int CIOSensor::configure()
{
  int dtypeSize;
  int ret;

#ifdef CONFIG_SENSORS_USE_B16
  if (getDtype() != SObjectId::DTYPE_B16)
    {
      DAWNERR("Sensor requires DTYPE_B16 when CONFIG_SENSORS_USE_B16 is enabled\n");
      return -EINVAL;
    }
#else
  if (getDtype() != SObjectId::DTYPE_FLOAT)
    {
      DAWNERR("Sensor requires DTYPE_FLOAT when CONFIG_SENSORS_USE_FLOAT is enabled\n");
      return -EINVAL;
    }
#endif

  dtypeSize = SObjectId::getDtypeSize_(static_cast<SObjectId::EObjectDataType>(getDtype()));
  if (dtypeSize != sizeof(sensor_data_t))
    {
      DAWNERR("Sensor dtype size %d does not match NuttX sensor data size %zu\n",
              dtypeSize,
              sizeof(sensor_data_t));
      return -EINVAL;
    }

  dsize = dtypeSize;

  if (info == nullptr)
    {
      DAWNERR("Unsupported sensor class %d\n", getCls());
      return -EINVAL;
    }

  if (DATA_BUFFER_SIZE < info->rsize)
    {
      DAWNERR("Sensor data size %zu exceeds buffer size %zu\n", info->rsize, DATA_BUFFER_SIZE);
      return -EINVAL;
    }

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Sensor configure failed (error %d)\n", ret);
      return ret;
    }

  // Get path to sensor

  if (getCmnDevno() == -1)
    {
      DAWNERR("SENSOR device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), SENSOR_PATH_FMT, info->path, "", getCmnDevno());

  // Open file

  fd = sensor_open(path);
  if (fd < 0)
    {
      DAWNERR("failed to open file %d\n", -errno);
      return -errno;
    }

  return OK;
}

int CIOSensor::deinit()
{
  // Close file

  sensor_close(fd);
  return OK;
}

int CIOSensor::getDataImpl(IODataCmn &data, size_t len)
{
  uint8_t buf[DATA_BUFFER_SIZE];
  size_t rsize = info->rsize;
  int ret;

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  // Read data into temp buffer (kernel struct always includes timestamp)

  ret = sensor_read(fd, buf, rsize);
  if (ret < 0)
    {
      DAWNERR("sensor_read failed %d\n", ret);
      return ret;
    }

  // Copy data portion (skip kernel timestamp)

  std::memcpy(data.getDataPtr(), buf + info->payloadOffset, data.getDataSize());

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      io_ts_t *kts = reinterpret_cast<io_ts_t *>(buf);
      data.getTs() = *kts;
    }
#endif

  return OK;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIOSensor::getFd() const
{
  return fd;
}
#endif

size_t CIOSensor::getDataSize() const
{
  return info->dsize * dsize;
}

size_t CIOSensor::getDataDim() const
{
  return info->dsize;
}
