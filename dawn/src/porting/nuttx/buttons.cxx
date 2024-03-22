// dawn/src/porting/nuttx/buttons.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>
#include <fcntl.h>
#include <nuttx/input/buttons.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: buttons_open
//***************************************************************************

int buttons_open(const char *path)
{
  int fd;

#ifdef CONFIG_DAWN_NUTTX_BOARDS_COMPAT
  path = "/dev/buttons";
#endif

  fd = open(path, O_RDWR);
  DAWNINFO("BUTTONS: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open BUTTONS file %s (error %d)\n", path, fd);
      return -EIO;
    }
  return fd;
}

//***************************************************************************
// Name: buttons_close
//***************************************************************************

void buttons_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: buttons_read
//***************************************************************************

int buttons_read(int fd, uint32_t *buttons)
{
  return static_cast<int>(read(fd, buttons, sizeof(btn_buttonset_t)));
}
