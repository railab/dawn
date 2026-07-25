// dawn/src/porting/nuttx/pot.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/pot.hxx"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/analog/pot.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: pot_open
//***************************************************************************

int pot_open(const char *path)
{
  int fd;

  fd = open(path, O_RDWR | O_NONBLOCK);
  DAWNINFO("POT: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open POT file %s (error %d)\n", path, fd);
      return -EIO;
    }
  return fd;
}

//***************************************************************************
// Name: pot_close
//***************************************************************************

void pot_close(int fd)
{
  if (fd)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: pot_get_max
//***************************************************************************

int pot_get_max(int fd, uint32_t *max)
{
  struct pot_info_s info;
  int ret;

  ret = ioctl(fd, POTIOC_GET_INFO, reinterpret_cast<unsigned long>(&info));
  if (ret >= 0)
    {
      *max = info.max;
    }

  return ret;
}

//***************************************************************************
// Name: pot_set_wiper
//***************************************************************************

int pot_set_wiper(int fd, dawn::porting::pot_rw_s *pot)
{
  struct pot_wiper_s wiper;
  int ret;

  wiper.wiper = pot->wiper;
  wiper.val = pot->val;

  ret = ioctl(fd, POTIOC_SET_WIPER, reinterpret_cast<unsigned long>(&wiper));
  if (ret < 0)
    {
      return -errno;
    }

  return ret;
}

//***************************************************************************
// Name: pot_get_wiper
//***************************************************************************

int pot_get_wiper(int fd, dawn::porting::pot_rw_s *pot)
{
  struct pot_wiper_s wiper;
  int ret;

  wiper.wiper = pot->wiper;
  wiper.val = 0;

  ret = ioctl(fd, POTIOC_GET_WIPER, reinterpret_cast<unsigned long>(&wiper));
  if (ret < 0)
    {
      return -errno;
    }

  pot->val = wiper.val;
  return ret;
}
