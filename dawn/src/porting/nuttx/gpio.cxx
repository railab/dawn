// dawn/src/porting/nuttx/gpio.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>
#include <fcntl.h>
#include <nuttx/ioexpander/gpio.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: gpi_open
//***************************************************************************

int gpi_open(const char *path)
{
  int fd;

  fd = open(path, O_RDWR);
  DAWNINFO("GPI: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open GPI file %s (error %d)\n", path, fd);
      return -EIO;
    }
  return fd;
}

//***************************************************************************
// Name: gpi_close
//***************************************************************************

void gpi_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: gpi_init
//***************************************************************************

int gpi_init(int fd)
{
  int pintype = 0;
  int ret;

  // Make sure that this is input pin

  ret = ioctl(fd, GPIOC_PINTYPE, reinterpret_cast<unsigned long>(&pintype));
  if (ret < 0)
    {
      DAWNERR("GPI ioctl GPIOC_PINTYPE failed (error %d)\n", ret);
      return -EIO;
    }

  if (pintype == GPIO_OUTPUT_PIN)
    {
      DAWNERR("GPIO pin is output, expected input (pintype %d)\n", pintype);
      return -EINVAL;
    }

  return OK;
}

//***************************************************************************
// Name: gpi_read
//***************************************************************************

int gpi_read(int fd, bool *invalue)
{
  int ret;

  ret = ioctl(fd, GPIOC_READ, reinterpret_cast<unsigned long>(invalue));
  if (ret < 0)
    {
      DAWNERR("GPIOC_READ failed %d\n", -errno);
      return -errno;
    }

  return ret;
}

//***************************************************************************
// Name: gpi_notify
//***************************************************************************

bool gpi_notify(int fd)
{
  return ioctl(fd, GPIOC_REGISTER, nullptr) < 0 ? false : true;
}

//***************************************************************************
// Name: gpo_open
//***************************************************************************

int gpo_open(const char *path)
{
  int fd;

  fd = open(path, O_RDWR);
  DAWNINFO("GPO: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open GPO file %s (error %d)\n", path, fd);
      return -EIO;
    }
  return fd;
}

//***************************************************************************
// Name: gpo_close
//***************************************************************************

void gpo_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: gpo_init
//***************************************************************************

int gpo_init(int fd)
{
  int pintype = 0;
  int ret;

  // Make sure that this is output pin

  ret = ioctl(fd, GPIOC_PINTYPE, reinterpret_cast<unsigned long>(&pintype));
  if (ret < 0)
    {
      DAWNERR("GPO ioctl GPIOC_PINTYPE failed (error %d)\n", ret);
      return -EIO;
    }

  if (pintype != GPIO_OUTPUT_PIN)
    {
      DAWNERR("GPIO pin is input, expected output (pintype %d)\n", pintype);
      return -EINVAL;
    }

  return OK;
}

//***************************************************************************
// Name: gpo_read
//***************************************************************************

int gpo_read(int fd, bool *invalue)
{
  int ret;

  ret = ioctl(fd, GPIOC_READ, reinterpret_cast<unsigned long>(invalue));
  if (ret < 0)
    {
      DAWNERR("GPIOC_READ failed %d\n", -errno);
      return -errno;
    }

  return ret;
}

//***************************************************************************
// Name: gpo_write
//***************************************************************************

int gpo_write(int fd, bool invalue)
{
  int ret;

  ret = ioctl(fd, GPIOC_WRITE, static_cast<unsigned long>(invalue));
  if (ret < 0)
    {
      DAWNERR("GPIOC_WRITE failed %d\n", -errno);
      return -errno;
    }

  return ret;
}
