// dawn/src/porting/nuttx/rgbled.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/rgbled.hxx"

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include "dawn/debug.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: rgbled_open
//***************************************************************************

int rgbled_open(const char *path)
{
  int fd;

  fd = open(path, O_RDWR);
  DAWNINFO("RGBLED: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open RGBLED file %s (error %d)\n", path, fd);
      return -EIO;
    }

  return fd;
}

//***************************************************************************
// Name: rgbled_close
//***************************************************************************

void rgbled_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: rgbled_write
//***************************************************************************

int rgbled_write(int fd, const char *rgb, size_t len)
{
  ssize_t ret;

  ret = write(fd, rgb, len);
  if (ret < 0)
    {
      return -errno;
    }

  if (static_cast<size_t>(ret) != len)
    {
      return -EIO;
    }

  return OK;
}
