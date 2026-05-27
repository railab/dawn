// dawn/src/porting/nuttx/pulsecount.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/pulsecount.hxx"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/timers/pulsecount.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"

int pulsecount_open(const char *path)
{
  int fd;

  fd = open(path, O_WRONLY | O_NONBLOCK);
  DAWNINFO("pulsecount: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open pulsecount file %s (error %d)\n", path, fd);
      return -EIO;
    }

  return fd;
}

void pulsecount_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

void pulsecount_init(int fd)
{
  (void)fd;
}

int pulsecount_start(int fd)
{
  return ioctl(fd, PULSECOUNTIOC_START, 0);
}

int pulsecount_stop(int fd)
{
  return ioctl(fd, PULSECOUNTIOC_STOP, 0);
}

int pulsecount_write(int fd, const dawn::porting::pulsecount_write_s *pulsecount)
{
  struct pulsecount_info_s info = {0};
  int ret;

  info.high_ns = pulsecount->high_ns;
  info.low_ns = pulsecount->low_ns;
  info.count = pulsecount->count;

  ret = ioctl(fd, PULSECOUNTIOC_SETCHARACTERISTICS, reinterpret_cast<unsigned long>(&info));
  if (ret < 0)
    {
      DAWNERR("PULSECOUNTIOC_SETCHARACTERISTICS failed %d\n", -errno);
      return -errno;
    }

  return ret;
}
