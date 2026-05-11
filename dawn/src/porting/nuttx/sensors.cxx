// dawn/src/porting/nuttx/sensors.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>
#include <fcntl.h>
#include <nuttx/sensors/sensor.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <cstdint>
#include <cstring>

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

#ifdef CONFIG_USENSOR
//***************************************************************************
// Name: sensor_user_register
//***************************************************************************

int sensor_user_register(const char *path, size_t event_size, uint32_t queue_size, bool persist)
{
  struct sensor_reginfo_s reginfo;
  int fd;
  int ret;
  int errcode;

  std::memset(&reginfo, 0, sizeof(reginfo));
  strlcpy(reginfo.path, path, sizeof(reginfo.path));
  reginfo.esize = event_size;
  reginfo.nbuffer = queue_size;
  reginfo.persist = persist;

  fd = open("/dev/usensor", O_CLOEXEC | O_WRONLY);
  if (fd < 0)
    {
      DAWNERR("Failed to open /dev/usensor (error %d)\n", -errno);
      return -errno;
    }

  ret =
    ioctl(fd, SNIOC_REGISTER, static_cast<unsigned long>(reinterpret_cast<uintptr_t>(&reginfo)));
  errcode = errno;
  close(fd);
  if (ret < 0)
    {
      return -errcode;
    }

  return OK;
}

//***************************************************************************
// Name: sensor_user_unregister
//***************************************************************************

int sensor_user_unregister(const char *path)
{
  int fd;
  int ret;
  int errcode;

  fd = open("/dev/usensor", O_CLOEXEC | O_WRONLY);
  if (fd < 0)
    {
      return -errno;
    }

  ret = ioctl(fd, SNIOC_UNREGISTER, static_cast<unsigned long>(reinterpret_cast<uintptr_t>(path)));
  errcode = errno;
  close(fd);
  if (ret < 0)
    {
      return -errcode;
    }

  return OK;
}
#endif

//***************************************************************************
// Name: sensor_open_write
//***************************************************************************

int sensor_open_write(const char *path)
{
  int fd;

  fd = open(path, O_CLOEXEC | O_WRONLY | O_NONBLOCK);
  DAWNINFO("CIOSensorProducer: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open SENSOR producer file %s (error %d)\n", path, fd);
      return -errno;
    }

  return fd;
}

//***************************************************************************
// Name: sensor_write
//***************************************************************************

int sensor_write(int fd, const void *data, size_t len)
{
  ssize_t ret;

  ret = write(fd, data, len);
  if (ret < 0)
    {
      DAWNERR("write failed %d\n", -errno);
      return -errno;
    }

  return static_cast<int>(ret);
}
