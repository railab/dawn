// dawn/src/porting/nuttx/leds.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/leds.hxx"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/leds/userled.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: leds_open
//***************************************************************************

int leds_open(const char *path)
{
  int fd;

#ifdef CONFIG_DAWN_NUTTX_BOARDS_COMPAT
  path = "/dev/userled";
#endif

  fd = open(path, O_RDWR);
  DAWNINFO("LEDS: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open LEDS file %s (error %d)\n", path, fd);
      return -EIO;
    }
  return fd;
}

//***************************************************************************
// Name: leds_close
//***************************************************************************

void leds_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: leds_read
//***************************************************************************

int leds_read(int fd, uint32_t *leds)
{
  return ioctl(fd, ULEDIOC_GETALL, reinterpret_cast<unsigned long>(leds));
}

//***************************************************************************
// Name: leds_write
//***************************************************************************

int leds_write(int fd, uint32_t leds)
{
  return ioctl(fd, ULEDIOC_SETALL, leds);
}
