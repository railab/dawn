// dawn/src/porting/nuttx/dac.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/dac.hxx"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/analog/dac.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: dac_open
//***************************************************************************

int dac_open(const char *path)
{
  int fd;

  fd = open(path, O_WRONLY | O_NONBLOCK);
  DAWNINFO("DAC: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open DAC file %s (error %d)\n", path, fd);
      return -EIO;
    }
  return fd;
}

//***************************************************************************
// Name: dac_close
//***************************************************************************

void dac_close(int fd)
{
  if (fd)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: dac_init
//***************************************************************************

void dac_init(int fd)
{
  // Channel validation is the IO layer's responsibility.

  (void)fd;
}

//***************************************************************************
// Name: dac_write
//***************************************************************************

int dac_write(int fd, dawn::porting::dac_write_s *dac)
{
  struct dac_msg_s msg;

  msg.am_channel = 0;
  msg.am_data = dac->data;

  return static_cast<int>(write(fd, &msg, sizeof(struct dac_msg_s)));
}
