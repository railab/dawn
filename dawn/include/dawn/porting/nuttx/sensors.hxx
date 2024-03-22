// dawn/include/dawn/porting/nuttx/sensors.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <nuttx/sensors/sensor.h>

#include <map>

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
  template<typename T>
  static constexpr size_t sensor_dsize()
  {
    static_assert(sizeof(T) >= sizeof(io_ts_t), "sensor struct smaller than timestamp");
    static_assert((sizeof(T) - sizeof(io_ts_t)) % sizeof(sensor_data_t) == 0,
                  "sensor payload not aligned to sensor data size");
    return (sizeof(T) - sizeof(io_ts_t)) / sizeof(sensor_data_t);
  }

  // Sensor map info

  struct io_sensor_map_info_s
  {
    const char *path; // Sensor path
    size_t rsize;     // Read data size
    size_t dsize;     // Data size
  } typedef SIOSensorMapInfo;

  // Get sensor info

  SIOSensorMapInfo *getSensorInfo(int16_t cls)
  {
    return &this->_map[cls];
  }

private:
  // Sensors info map

  std::map<int, SIOSensorMapInfo> _map = {
    {CIOCommon::IO_CLASS_SENSOR_ACCELEROMETER,
     {
       "accel",
       sizeof(struct sensor_accel),
       sensor_dsize<struct sensor_accel>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_MAGNETICFIELD,
     {
       "mag",
       sizeof(struct sensor_mag),
       sensor_dsize<struct sensor_mag>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_GYROSCOPE,
     {
       "gyro",
       sizeof(struct sensor_gyro),
       sensor_dsize<struct sensor_gyro>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_LIGHT,
     {
       "light",
       sizeof(struct sensor_light),
       sensor_dsize<struct sensor_light>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_BAROMETER,
     {
       "baro",
       sizeof(struct sensor_baro),
       sensor_dsize<struct sensor_baro>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_PROXIMITY,
     {
       "prox",
       sizeof(struct sensor_prox),
       sensor_dsize<struct sensor_prox>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_HUMIDITY,
     {
       "humi",
       sizeof(struct sensor_humi),
       sensor_dsize<struct sensor_humi>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_TEMPERATURE,
     {
       "temp",
       sizeof(struct sensor_temp),
       sensor_dsize<struct sensor_temp>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_ATEMPERATURE,
     {
       "ambient_temp",
       sizeof(struct sensor_temp),
       sensor_dsize<struct sensor_temp>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_RGB,
     {
       "rgb",
       sizeof(struct sensor_rgb),
       sensor_dsize<struct sensor_rgb>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_IR,
     {
       "ir",
       sizeof(struct sensor_ir),
       sensor_dsize<struct sensor_ir>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_UV,
     {
       "uv",
       sizeof(struct sensor_uv),
       sensor_dsize<struct sensor_uv>(),
     }},
    {CIOCommon::IO_CLASS_SENSOR_GAS,
     {
       "gas",
       sizeof(struct sensor_gas),
       sensor_dsize<struct sensor_gas>(),
     }},
  };
};
} // namespace dawn
