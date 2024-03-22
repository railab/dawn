// dawn/src/porting/nuttx/pwm.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/pwm.hxx"

#include <climits>
#include <errno.h>
#include <fcntl.h>
#include <nuttx/timers/pwm.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: pwm_open
//***************************************************************************

int pwm_open(const char *path)
{
  int fd;

  fd = open(path, O_WRONLY | O_NONBLOCK);
  DAWNINFO("PWM: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open PWM file %s (error %d)\n", path, fd);
      return -EIO;
    }

  return fd;
}

//***************************************************************************
// Name: pwm_close
//***************************************************************************

void pwm_close(int fd)
{
  if (fd)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: pwm_init
//***************************************************************************

void pwm_init(int fd)
{
  // Channel validation is the IO layer's responsibility.

  (void)fd;
}

//***************************************************************************
// Name: pwm_start
//***************************************************************************

int pwm_start(int fd)
{
  return ioctl(fd, PWMIOC_START, 0);
}

//***************************************************************************
// Name: pwm_stop
//***************************************************************************

int pwm_stop(int fd)
{
  return ioctl(fd, PWMIOC_STOP, 0);
}

//***************************************************************************
// Name: pwm_write
//***************************************************************************

int pwm_write(int fd, dawn::porting::pwm_write_s *pwm)
{
  struct pwm_info_s info = {0};
  int ret;

  // Fill data

  info.frequency = pwm->freq;

  for (int i = 0; i < CONFIG_PWM_NCHANNELS; i++)
    {
      info.channels[i].duty = b16idiv(pwm->channels[i].duty, 1000);
      if (pwm->channels[i].channel > SCHAR_MAX)
        {
          return -ERANGE;
        }

      info.channels[i].channel = static_cast<int8_t>(pwm->channels[i].channel);
    }

  // Set characteristic

  ret = ioctl(fd, PWMIOC_SETCHARACTERISTICS, reinterpret_cast<unsigned long>(&info));
  if (ret < 0)
    {
      DAWNERR("PWMIOC_SETCHARACTERISTICS failed %d\n", -errno);
      return -errno;
    }

  return ret;
}
