// dawn/include/dawn/io/sensor_producer.hxx
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
 * @brief Sensor/uORB publisher backed by Dawn protocol writes.
 *
 * CIOSensorProducer creates a NuttX user sensor topic and writes incoming
 * Dawn data into the matching NuttX sensor event structure.
 */

class CIOSensorProducer
  : public CIOCommon
  , public CIOSensorPorting
{
public:
  enum
  {
    IO_SENSOR_PRODUCER_CFG_FIRST = 0,
    IO_SENSOR_PRODUCER_CFG_QUEUE_SIZE = 1,
    IO_SENSOR_PRODUCER_CFG_PERSIST = 2,
    IO_SENSOR_PRODUCER_CFG_LAST = 31
  };

  static_assert(IO_SENSOR_PRODUCER_CFG_LAST - 1 <= SObjectCfg::ID_MAX);

  explicit CIOSensorProducer(CDescObject &desc)
    : CIOCommon(desc)
    , info(getSensorInfo(producerToSensorClass(getCls())))
    , queueSize(1)
    , persist(false)
    , registered(false)
    , ts(0)
    , fd(-1)
  {
  }

  ~CIOSensorProducer() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    switch (getCls())
      {
        case IO_CLASS_SENSOR_PRODUCER_ACCELEROMETER:
          return "sprod_accel";
        case IO_CLASS_SENSOR_PRODUCER_MAGNETICFIELD:
          return "sprod_mag";
        case IO_CLASS_SENSOR_PRODUCER_GYROSCOPE:
          return "sprod_gyro";
        case IO_CLASS_SENSOR_PRODUCER_LIGHT:
          return "sprod_light";
        case IO_CLASS_SENSOR_PRODUCER_BAROMETER:
          return "sprod_baro";
        case IO_CLASS_SENSOR_PRODUCER_PROXIMITY:
          return "sprod_prox";
        case IO_CLASS_SENSOR_PRODUCER_HUMIDITY:
          return "sprod_hum";
        case IO_CLASS_SENSOR_PRODUCER_TEMPERATURE:
          return "sprod_temp";
        case IO_CLASS_SENSOR_PRODUCER_ATEMPERATURE:
          return "sprod_atemp";
        case IO_CLASS_SENSOR_PRODUCER_RGB:
          return "sprod_rgb";
        case IO_CLASS_SENSOR_PRODUCER_IR:
          return "sprod_ir";
        case IO_CLASS_SENSOR_PRODUCER_UV:
          return "sprod_uv";
        case IO_CLASS_SENSOR_PRODUCER_GAS:
          return "sprod_gas";
        default:
          return "sprod";
      }
  }
#endif

  int configure() override;
  int deinit() override;
  int setDataImpl(IODataCmn &data) override;

  size_t getDataSize() const override;
  size_t getDataDim() const override;

  bool isRead() const override
  {
    return false;
  };

  bool isWrite() const override
  {
    return true;
  };

  bool isNotify() const override
  {
    return false;
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
    return CIOSensorProducer::objectId(
      CIOCommon::IO_CLASS_SENSOR_PRODUCER_ACCELEROMETER, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdMag(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(
      CIOCommon::IO_CLASS_SENSOR_PRODUCER_MAGNETICFIELD, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdGyro(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(
      CIOCommon::IO_CLASS_SENSOR_PRODUCER_GYROSCOPE, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdLight(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(CIOCommon::IO_CLASS_SENSOR_PRODUCER_LIGHT, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdBaro(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(
      CIOCommon::IO_CLASS_SENSOR_PRODUCER_BAROMETER, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdProx(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(
      CIOCommon::IO_CLASS_SENSOR_PRODUCER_PROXIMITY, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdHum(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(
      CIOCommon::IO_CLASS_SENSOR_PRODUCER_HUMIDITY, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdTemp(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(
      CIOCommon::IO_CLASS_SENSOR_PRODUCER_TEMPERATURE, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdAtemp(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(
      CIOCommon::IO_CLASS_SENSOR_PRODUCER_ATEMPERATURE, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdRgb(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(CIOCommon::IO_CLASS_SENSOR_PRODUCER_RGB, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdIr(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(CIOCommon::IO_CLASS_SENSOR_PRODUCER_IR, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdUv(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(CIOCommon::IO_CLASS_SENSOR_PRODUCER_UV, dtype, ts, inst);
  }

  constexpr static SObjectId::ObjectId objectIdGas(uint8_t dtype, bool ts, uint16_t inst)
  {
    return CIOSensorProducer::objectId(CIOCommon::IO_CLASS_SENSOR_PRODUCER_GAS, dtype, ts, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdQueueSize()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_SENSOR_PRODUCER_CONFIG,
                                 SObjectId::DTYPE_UINT32,
                                 true,
                                 1,
                                 IO_SENSOR_PRODUCER_CFG_QUEUE_SIZE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPersist()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_SENSOR_PRODUCER_CONFIG,
                                 SObjectId::DTYPE_BOOL,
                                 true,
                                 1,
                                 IO_SENSOR_PRODUCER_CFG_PERSIST);
  }

private:
  constexpr static const size_t DATA_BUFFER_SIZE = 32;

  const SIOSensorMapInfo *info; ///< Sensor metadata for the produced event.
  uint32_t queueSize;           ///< Number of uORB events buffered by the topic.
  bool persist;                 ///< Keep latest event readable for late subscribers.
  bool registered;              ///< True if this instance created the user sensor.
  uint64_t ts;                  ///< Timestamp of the last published event.
  char path[PATH_MAX] = {};
  int fd;

  int configureDesc(const CDescObject &desc);
};
} // namespace dawn
