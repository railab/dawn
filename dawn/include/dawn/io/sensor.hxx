// dawn/include/dawn/io/sensor.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/porting/sensors.hxx"

namespace dawn
{
/**
 * @brief Generic sensor interface for various sensor types.
 *
 * Provides a unified interface for reading data from various sensor types
 * including accelerometers, magnetometers, gyroscopes, light sensors,
 * barometers, proximity sensors, humidity sensors, temperature sensors, RGB
 * sensors, IR sensors, UV sensors, and gas sensors.
 */

class CIOSensor
  : public CIOCommon
  , public CIOSensorPorting
{
public:
  enum
  {
    IO_SENSOR_CFG_FIRST = 0,
    IO_SENSOR_CFG_ENABLE = 1,         ///< Sensor enable/disable configuration.
    IO_SENSOR_CFG_UNIT = 2,           ///< Measurement unit selection.
    IO_SENSOR_CFG_GAIN = 3,           ///< Analog gain setting.
    IO_SENSOR_CFG_SCALE = 4,          ///< Scale/multiplier for raw sensor values.
    IO_SENSOR_CFG_UPDATEINTERVAL = 5, ///< Data update interval in milliseconds.
    IO_SENSOR_CFG_MEASPERIOD = 6,     ///< Measurement period.
    IO_SENSOR_CFG_BANDWIDTHLP = 7,    ///< Low-pass filter bandwidth.
    IO_SENSOR_CFG_BANDWIDTHHP = 8,    ///< High-pass filter bandwidth.
    IO_SENSOR_CFG_ORIENTATION = 9,    ///< Sensor orientation/mounting angle.
    IO_SENSOR_CFG_ALERT = 10,         ///< Alert threshold configuration.
    IO_SENSOR_CFG_LAST = 31
  };

  static_assert(IO_SENSOR_CFG_LAST - 1 <= SObjectCfg::ID_MAX);

  explicit CIOSensor(CDescObject &desc)
    : CIOCommon(desc)
    , info(getSensorInfo(getCls()))
    , dsize(4)
    , updateInterval(0)
    , measurementPeriod(0)
    , fd(-1)
  {
  }

  ~CIOSensor() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    switch (getCls())
      {
        case IO_CLASS_SENSOR_ACCELEROMETER:
          return "accel";
        case IO_CLASS_SENSOR_MAGNETICFIELD:
          return "mag";
        case IO_CLASS_SENSOR_GYROSCOPE:
          return "gyro";
        case IO_CLASS_SENSOR_LIGHT:
          return "light";
        case IO_CLASS_SENSOR_BAROMETER:
          return "baro";
        case IO_CLASS_SENSOR_PROXIMITY:
          return "prox";
        case IO_CLASS_SENSOR_HUMIDITY:
          return "humid";
        case IO_CLASS_SENSOR_TEMPERATURE:
          return "temp";
        case IO_CLASS_SENSOR_ATEMPERATURE:
          return "atemp";
        case IO_CLASS_SENSOR_RGB:
          return "rgb";
        case IO_CLASS_SENSOR_IR:
          return "ir";
        case IO_CLASS_SENSOR_UV:
          return "uv";
        case IO_CLASS_SENSOR_GAS:
          return "gas";
        case IO_CLASS_SENSOR_GNSS:
          return "gnss";
        case IO_CLASS_SENSOR_GNSS_TIME:
          return "gnss_time";
        case IO_CLASS_SENSOR_GNSS_INFO:
          return "gnss_info";
        case IO_CLASS_SENSOR_GNSS_SATELLITES:
          return "gnss_sats";
        default:
          return "sensor";
      }
  }
#endif

  int configure() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;

#ifdef CONFIG_DAWN_IO_NOTIFY
  int getFd() const override;
#endif

  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return true;
  };

  bool isWrite() const override
  {
    return false;
  };

  bool isNotify() const override
  {
    return true;
  };

  bool isBatch() const override
  {
    return false;
  };

  constexpr static SObjectId::ObjectId objectId(uint16_t cls, uint8_t dtype, bool ts, uint16_t inst)
  {
    uint8_t flags = 0;

#ifdef CONFIG_DAWN_IO_TIMESTAMP
    if (ts)
      {
        flags |= CIOCommon::IO_FLAGS_TS;
      }
#else
    DAWNASSERT(ts == false, "ts not supported");
#endif

    return SObjectId::objectId(SObjectId::OBJTYPE_IO, cls, dtype, flags, inst);
  }

  constexpr static SObjectId::ObjectId objectIdAccel(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_ACCELEROMETER, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdMag(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_MAGNETICFIELD, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdGyro(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_GYROSCOPE, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdLight(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_LIGHT, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdBaro(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_BAROMETER, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdProx(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_PROXIMITY, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdHum(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_HUMIDITY, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdTemp(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_TEMPERATURE, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdAtemp(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_ATEMPERATURE, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdRgb(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_RGB, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdIr(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_IR, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdUv(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_UV, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdGas(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_GAS, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdGnss(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_GNSS, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdGnssTime(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_GNSS_TIME, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdGnssInfo(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_GNSS_INFO, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdGnssSats(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensor::objectId(CIOCommon::IO_CLASS_SENSOR_GNSS_SATELLITES, dtype, ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdUpdateInterval()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_ANY,
                                 SObjectId::DTYPE_UINT32,
                                 true,
                                 1,
                                 IO_SENSOR_CFG_UPDATEINTERVAL);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdMeasurementPeriod()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_ANY,
                                 SObjectId::DTYPE_UINT32,
                                 true,
                                 1,
                                 IO_SENSOR_CFG_MEASPERIOD);
  }

protected:
  // Must fit the largest NuttX sensor event struct read in one go. The GNSS
  // event (struct sensor_gnss) is the largest at ~72 bytes; keep headroom.
  constexpr static const size_t DATA_BUFFER_SIZE = 96;

  const SIOSensorMapInfo *info; ///< Sensor metadata.
  size_t dsize;                 ///< Sensor data size in bytes.
  uint32_t updateInterval;      ///< Sensor update interval in milliseconds.
  uint32_t measurementPeriod;   ///< Sensor measurement period in milliseconds.
  char path[PATH_MAX] = {};     ///< Sensor device file path.
  int fd;                       ///< File descriptor for sensor device.

  // Validate the configured dtype and set dsize (bytes per element). The base
  // accepts only the NuttX float sensor_data_t; GNSS subclasses override this
  // to also accept integer time/satellite fields.
  virtual int validateDtype();

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
