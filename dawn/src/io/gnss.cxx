// dawn/src/io/gnss.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/gnss.hxx"

#include <cstdint>
#include <errno.h>

#include <nuttx/uorb.h>

#include "dawn/debug.hxx"
#include "dawn/porting/sensors.hxx"

using namespace dawn;

int CIOSensorGnss::validateDtype()
{
  // GNSS variants are float (position/info), uint64 (time) or uint32
  // (satellites). Accept those and record the element size.

  switch (getDtype())
    {
      case SObjectId::DTYPE_FLOAT:
      case SObjectId::DTYPE_UINT32:
      case SObjectId::DTYPE_UINT64:
        dsize = SObjectId::getDtypeSize_(static_cast<SObjectId::EObjectDataType>(getDtype()));
        return OK;
      default:
        DAWNERR("GNSS IO unsupported dtype %d\n", getDtype());
        return -EINVAL;
    }
}

int CIOSensorGnss::getDataImpl(IODataCmn &data, size_t len)
{
  struct sensor_gnss gnss;
  int ret;

  // No batch supported

  if (len != 1)
    {
      return -ENOTSUP;
    }

  // Read the full GNSS event from the device

  ret = sensor_read(fd, &gnss, sizeof(gnss));
  if (ret < 0)
    {
      DAWNERR("sensor_read failed %d\n", ret);
      return ret;
    }

  // Assemble the fields for this variant (non-adjacent struct members, with
  // type conversion for time/satellites).

  switch (getCls())
    {
      case IO_CLASS_SENSOR_GNSS:
        {
          // Position + velocity
          float *o = static_cast<float *>(data.getDataPtr());
          o[0] = gnss.latitude;
          o[1] = gnss.longitude;
          o[2] = gnss.altitude;
          o[3] = gnss.ground_speed;
          o[4] = gnss.course;
        }
        break;

      case IO_CLASS_SENSOR_GNSS_TIME:
        // time_utc is already UTC seconds since the epoch (the nRF91 driver
        // converts the GNSS calendar time with timegm()); LwM2M Time wants
        // seconds, so pass it through.
        *static_cast<uint64_t *>(data.getDataPtr()) = gnss.time_utc;
        break;

      case IO_CLASS_SENSOR_GNSS_INFO:
        {
          // Accuracy + dilution of precision
          float *o = static_cast<float *>(data.getDataPtr());
          o[0] = gnss.eph;
          o[1] = gnss.epv;
          o[2] = gnss.hdop;
          o[3] = gnss.pdop;
          o[4] = gnss.vdop;
        }
        break;

      case IO_CLASS_SENSOR_GNSS_SATELLITES:
        *static_cast<uint32_t *>(data.getDataPtr()) = gnss.satellites_used;
        break;

      default:
        return -EINVAL;
    }

#ifdef CONFIG_DAWN_IO_TIMESTAMP
  if (isTimestamp())
    {
      data.getTs() = static_cast<io_ts_t>(gnss.timestamp);
    }
#endif

  return OK;
}
