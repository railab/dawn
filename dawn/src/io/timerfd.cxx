// dawn/src/io/timerfd.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <inttypes.h>
#include <sys/timerfd.h>

#include "dawn/io/timestamp.hxx"

using namespace dawn;

CIOTimerfd::~CIOTimerfd()
{
  timfd_stop();

#ifdef CONFIG_DAWN_IO_NOTIFY
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }
#endif
};

int CIOTimerfd::timfd_init()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  // Use timer fd to handle interval

  if (interval > 0)
    {
      fd = timerfd_create(CLOCK_MONOTONIC, 0);
      if (fd < 0)
        {
          DAWNERR("timerfd_create failed errno=%d\n", errno);
          return -errno;
        }
    }
#endif
  return OK;
}

void CIOTimerfd::timfd_interval(uint32_t data)
{
  interval = data;
}

void CIOTimerfd::timfd_ack()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  if (fd >= 0)
    {
      timerfd_t tdret;
      read(fd, &tdret, sizeof(timerfd_t));
    }
#endif
}

int CIOTimerfd::timfd_fd() const
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  return fd;
#else
  return -1;
#endif
}

int CIOTimerfd::timfd_start()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  if (fd >= 0)
    {
      struct itimerspec tms;
      int ret;

      if (interval == 0)
        {
          return -EINVAL;
        }

      /* Start timers, interval is in us */

      tms.it_value.tv_sec = (interval / 1000000);
      tms.it_value.tv_nsec = static_cast<long>(interval % 1000000) * 1000;
      tms.it_interval.tv_sec = (interval / 1000000);
      tms.it_interval.tv_nsec = static_cast<long>(interval % 1000000) * 1000;

      ret = timerfd_settime(fd, 0, &tms, NULL);
      if (ret != OK)
        {
          DAWNERR("timerfd_settime for interval=%" PRIu32 " failed errno=%d\n", interval, errno);
        }

      return ret;
    }
#endif

  return OK;
}

int CIOTimerfd::timfd_stop()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  if (fd >= 0)
    {
      struct itimerspec tms;
      int ret;

      tms.it_value.tv_sec = 0;
      tms.it_value.tv_nsec = 0;
      tms.it_interval.tv_sec = 0;
      tms.it_interval.tv_nsec = 0;

      ret = timerfd_settime(fd, 0, &tms, NULL);
      if (ret != OK)
        {
          DAWNERR("timerfd_settime stop failed errno=%d\n", errno);
        }

      return ret;
    }
#endif

  return OK;
}
