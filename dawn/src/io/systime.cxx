// dawn/src/io/systime.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/systime.hxx"

#include <errno.h>
#include <time.h>

using namespace dawn;

int CIOSystime::init()
{
  return OK;
}

int CIOSystime::getDataImpl(IODataCmn &data, size_t len)
{
  struct timespec ts;
  int ret;

  if (len != 1)
    {
      return -ENOTSUP;
    }

  ret = clock_gettime(CLOCK_REALTIME, &ts);
  if (ret < 0)
    {
      return -errno;
    }

  // Return nanoseconds since epoch

  uint64_t *time_data = static_cast<uint64_t *>(data.getDataPtr());
  *time_data = static_cast<uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<uint64_t>(ts.tv_nsec);

  return OK;
}

int CIOSystime::setDataImpl(IODataCmn &data)
{
  struct timespec ts;
  int ret;

  // Set time from nanoseconds since epoch

  uint64_t *time_data = static_cast<uint64_t *>(data.getDataPtr());
  ts.tv_sec = static_cast<time_t>(*time_data / 1000000000ULL);
  ts.tv_nsec = static_cast<long>(*time_data % 1000000000ULL);

  ret = clock_settime(CLOCK_REALTIME, &ts);
  if (ret < 0)
    {
      return -errno;
    }

  return OK;
}

size_t CIOSystime::getDataSize() const
{
  return sizeof(uint64_t);
}

size_t CIOSystime::getDataDim() const
{
  return 1;
}

bool CIOSystime::isRead() const
{
  return true;
}

bool CIOSystime::isWrite() const
{
  return true;
}

bool CIOSystime::isNotify() const
{
  return false;
}

bool CIOSystime::isBatch() const
{
  return false;
}
