// dawn/src/porting/nuttx/sensors.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>
#include <fcntl.h>
#include <nuttx/sensors/sensor.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: sensor_open
//***************************************************************************

int sensor_open(const char *path)
{
  int fd;

  fd = open(path, O_CLOEXEC | O_RDWR | O_NONBLOCK);
  DAWNINFO("CIOSensor: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open SENSOR file %s (error %d)\n", path, fd);
      return -EIO;
    }
  return fd;
}

//***************************************************************************
// Name: sensor_close
//***************************************************************************

void sensor_close(int fd)
{
  if (fd)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: sensor_read
//***************************************************************************

int sensor_read(int fd, void *data, size_t len)
{
  ssize_t ret;

  ret = read(fd, data, len);
  if (ret < 0)
    {
      DAWNERR("read failed %d\n", -errno);
      return -errno;
    }

  return static_cast<int>(ret);
}
