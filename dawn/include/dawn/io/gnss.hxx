// dawn/include/dawn/io/gnss.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/io/sensor.hxx"

namespace dawn
{
/**
 * @brief GNSS sensor I/O.
 *
 * Reuses CIOSensor's device open / read / notify machinery, but reads the full
 * NuttX struct sensor_gnss and assembles the fields for the requested variant
 * (selected by IO class):
 *   - IO_CLASS_SENSOR_GNSS            position + velocity (lat, lon, alt,
 *                                     ground speed, course) as float[5];
 *   - IO_CLASS_SENSOR_GNSS_TIME       UTC time in seconds since epoch as
 *                                     uint64 (the kernel value is microseconds);
 *   - IO_CLASS_SENSOR_GNSS_INFO       accuracy + dilution of precision (eph,
 *                                     epv, hdop, pdop, vdop) as float[5];
 *   - IO_CLASS_SENSOR_GNSS_SATELLITES number of satellites used as uint32.
 *
 * Because the variants use different (and non-float) dtypes, the base
 * float-only dtype check is overridden.
 */

class CIOSensorGnss : public CIOSensor
{
public:
  explicit CIOSensorGnss(CDescObject &desc)
    : CIOSensor(desc)
  {
  }

protected:
  int validateDtype() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
};
} // Namespace dawn
