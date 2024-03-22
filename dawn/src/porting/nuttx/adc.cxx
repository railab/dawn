// dawn/src/porting/nuttx/adc.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/adc.hxx"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/analog/adc.h>
#include <nuttx/analog/ioctl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"

//***************************************************************************
// Public Functions
//***************************************************************************

//***************************************************************************
// Name: adc_open
//***************************************************************************

int adc_open(const char *path)
{
  int fd;

  fd = open(path, O_RDWR);
  DAWNINFO("ADC: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open ADC file %s (error %d)\n", path, errno);
      return -errno;
    }
  return fd;
}

//***************************************************************************
// Name: adc_close
//***************************************************************************

void adc_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

//***************************************************************************
// Name: adc_get_info
//***************************************************************************

int adc_get_nchans(int fd)
{
  int ret;
  ret = ioctl(fd, ANIOC_GET_NCHANNELS, nullptr);
  if (ret < 0)
    {
      return -errno;
    }

  // All channels must fit into FIFO

  if (CONFIG_ADC_FIFOSIZE <= ret)
    {
      DAWNERR("ADC channel count %d exceeds FIFO size %d\n", ret, CONFIG_ADC_FIFOSIZE);
      return -EINVAL;
    }

  return ret;
}

//***************************************************************************
// Name: adc_start
//***************************************************************************

int adc_start(int fd)
{
  int ret;

  ret = ioctl(fd, ANIOC_TRIGGER, 0);
  if (ret < 0)
    {
      DAWNERR("ANIOC_TRIGGER failed %d\n", errno);
      return -errno;
    }

  return OK;
}

//***************************************************************************
// Name: adc_stop
//***************************************************************************

int adc_stop(int fd)
{
  int ret;

  ret = ioctl(fd, ANIOC_STOP, 0);
  if (ret < 0)
    {
      if (errno == ENOTTY)
        {
          return -ENOSYS;
        }

      DAWNERR("ANIOC_STOP failed %d\n", errno);
      return -errno;
    }

  return OK;
}

//***************************************************************************
// Name: adc_set_timer_freq
//***************************************************************************

int adc_set_timer_freq(int fd, uint32_t freq_hz)
{
  int ret;

  ret = ioctl(fd, ANIOC_SET_TIMER_FREQ, freq_hz);
  if (ret < 0)
    {
      if (errno == ENOTTY)
        {
          return -ENOSYS;
        }

      DAWNERR("ANIOC_SET_TIMER_FREQ failed %d\n", errno);
      return -errno;
    }

  return OK;
}

//***************************************************************************
// Name: adc_get_samples_count
//***************************************************************************

int adc_get_samples_count(int fd)
{
  int ret;

  ret = ioctl(fd, ANIOC_SAMPLES_ON_READ, 0);
  if (ret < 0)
    {
      if (errno == ENOTTY)
        {
          return -ENOSYS;
        }

      return -errno;
    }

  return ret;
}

//***************************************************************************
// Name: adc_read
//***************************************************************************

int adc_read(int fd, dawn::porting::adc_read_s *adc, size_t len)
{
  ssize_t ret;

  // Read data from FIFO
  // NOTE: we read data without channel information!
  //       Correct order of samples must be done on board level!

  ret = read(fd, adc, len);
  if (ret < 0)
    {
      return -errno;
    }

  return static_cast<int>(ret);
}
