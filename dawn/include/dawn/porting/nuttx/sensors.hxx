// dawn/include/dawn/porting/nuttx/sensors.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <nuttx/sensors/sensor.h>

#include <cstddef>

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

// PATH format for sensor

#define SENSOR_PATH_FMT "/dev/uorb/sensor_%s%s%d"

namespace dawn
{
/** @class CIOSensorPorting */

class CIOSensorPorting
{
public:
  // Sensor map info

  struct io_sensor_map_info_s
  {
    int16_t cls;          // Dawn sensor class
    const char *path;     // NuttX sensor path suffix
    size_t rsize;         // Kernel event structure size
    size_t dsize;         // Number of sensor_data_t payload values
    size_t payloadOffset; // Offset of the first payload value in the event
  } typedef SIOSensorMapInfo;

  // Get sensor info

  static const SIOSensorMapInfo *getSensorInfo(int16_t cls)
  {
    static const SIOSensorMapInfo map[] = {
      {CIOCommon::IO_CLASS_SENSOR_ACCELEROMETER,
       "accel",
       sizeof(struct sensor_accel),
       4,
       offsetof(struct sensor_accel, x)},
      {CIOCommon::IO_CLASS_SENSOR_MAGNETICFIELD,
       "mag",
       sizeof(struct sensor_mag),
       4,
       offsetof(struct sensor_mag, x)},
      {CIOCommon::IO_CLASS_SENSOR_GYROSCOPE,
       "gyro",
       sizeof(struct sensor_gyro),
       4,
       offsetof(struct sensor_gyro, x)},
      {CIOCommon::IO_CLASS_SENSOR_LIGHT,
       "light",
       sizeof(struct sensor_light),
       2,
       offsetof(struct sensor_light, light)},
      {CIOCommon::IO_CLASS_SENSOR_BAROMETER,
       "baro",
       sizeof(struct sensor_baro),
       2,
       offsetof(struct sensor_baro, pressure)},
      {CIOCommon::IO_CLASS_SENSOR_PROXIMITY,
       "prox",
       sizeof(struct sensor_prox),
       1,
       offsetof(struct sensor_prox, proximity)},
      {CIOCommon::IO_CLASS_SENSOR_HUMIDITY,
       "humi",
       sizeof(struct sensor_humi),
       1,
       offsetof(struct sensor_humi, humidity)},
      {CIOCommon::IO_CLASS_SENSOR_TEMPERATURE,
       "temp",
       sizeof(struct sensor_temp),
       1,
       offsetof(struct sensor_temp, temperature)},
      {CIOCommon::IO_CLASS_SENSOR_ATEMPERATURE,
       "ambient_temp",
       sizeof(struct sensor_temp),
       1,
       offsetof(struct sensor_temp, temperature)},
      {CIOCommon::IO_CLASS_SENSOR_RGB,
       "rgb",
       sizeof(struct sensor_rgb),
       3,
       offsetof(struct sensor_rgb, r)},
      {CIOCommon::IO_CLASS_SENSOR_IR,
       "ir",
       sizeof(struct sensor_ir),
       1,
       offsetof(struct sensor_ir, ir)},
      {CIOCommon::IO_CLASS_SENSOR_UV,
       "uv",
       sizeof(struct sensor_uv),
       1,
       offsetof(struct sensor_uv, uvi)},
      {CIOCommon::IO_CLASS_SENSOR_GAS,
       "gas",
       sizeof(struct sensor_gas),
       1,
       offsetof(struct sensor_gas, gas_resistance)},
      // GNSS variants (handled by CIOSensorGnss). All read the same
      // /dev/uorb/sensor_gnss device; the payload offset is unused because
      // CIOSensorGnss::getDataImpl assembles the specific fields itself. Only
      // the element count (dim) matters here, for getDataSize().
      {CIOCommon::IO_CLASS_SENSOR_GNSS, // position+velocity: lat/lon/alt/speed/course
       "gnss",
       sizeof(struct sensor_gnss),
       5,
       0},
      {CIOCommon::IO_CLASS_SENSOR_GNSS_TIME, // UTC seconds (uint64)
       "gnss",
       sizeof(struct sensor_gnss),
       1,
       0},
      {CIOCommon::IO_CLASS_SENSOR_GNSS_INFO, // eph/epv/hdop/pdop/vdop
       "gnss",
       sizeof(struct sensor_gnss),
       5,
       0},
      {CIOCommon::IO_CLASS_SENSOR_GNSS_SATELLITES, // satellites used (uint32)
       "gnss",
       sizeof(struct sensor_gnss),
       1,
       0},
    };

    for (size_t i = 0; i < sizeof(map) / sizeof(map[0]); i++)
      {
        if (map[i].cls == cls)
          {
            return &map[i];
          }
      }

    return nullptr;
  }

  static int16_t producerToSensorClass(int16_t cls)
  {
    static_assert(CIOCommon::IO_CLASS_SENSOR_PRODUCER_GAS -
                      CIOCommon::IO_CLASS_SENSOR_PRODUCER_ACCELEROMETER ==
                    CIOCommon::IO_CLASS_SENSOR_GAS - CIOCommon::IO_CLASS_SENSOR_ACCELEROMETER,
                  "Sensor and sensor producer class ranges must stay aligned");

    if (cls < CIOCommon::IO_CLASS_SENSOR_PRODUCER_ACCELEROMETER ||
        cls > CIOCommon::IO_CLASS_SENSOR_PRODUCER_GAS)
      {
        return CIOCommon::IO_CLASS_ANY;
      }

    return cls - CIOCommon::IO_CLASS_SENSOR_PRODUCER_ACCELEROMETER +
           CIOCommon::IO_CLASS_SENSOR_ACCELEROMETER;
  }
};
} // namespace dawn
