// dawn/src/io/battery.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/battery.hxx"

#include <cstdint>
#include <cstdio>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/power/battery_ioctl.h>

using namespace dawn;

CIOBatteryBase::~CIOBatteryBase()
{
  deinit();
}

int CIOBatteryBase::configure()
{
  if (getDtype() != SObjectId::DTYPE_UINT32)
    {
      DAWNERR("Battery IO requires DTYPE_UINT32\n");
      return -EINVAL;
    }

  if (getCmnDevno() == -1)
    {
      DAWNERR("Battery device number not configured\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), "/dev/batt%d", getCmnDevno());

  fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      DAWNERR("failed to open %s (%d)\n", path, -errno);
      return -errno;
    }

  return OK;
}

int CIOBatteryBase::deinit()
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }

  return OK;
}

int CIOBatteryBase::batRead(int cmd, void *arg) const
{
  int ret = ioctl(fd, cmd, (unsigned long)(uintptr_t)arg);
  if (ret < 0)
    {
      return -errno;
    }

  return OK;
}

int CIOBattVolt::getDataImpl(IODataCmn &data, size_t len)
{
  int32_t raw;
  int ret;

  if (len != 1)
    {
      return -ENOTSUP;
    }

  ret = batRead(BATIOC_VOLTAGE, &raw);
  if (ret < 0)
    {
      return ret;
    }

  *reinterpret_cast<uint32_t *>(data.getDataPtr()) = static_cast<uint32_t>(raw);

  return OK;
}

int CIOBattSoc::getDataImpl(IODataCmn &data, size_t len)
{
  int32_t raw;
  int ret;

  if (len != 1)
    {
      return -ENOTSUP;
    }

  ret = batRead(BATIOC_CAPACITY, &raw);
  if (ret < 0)
    {
      return ret;
    }

  *reinterpret_cast<uint32_t *>(data.getDataPtr()) = static_cast<uint32_t>(raw);

  return OK;
}

int CIOBattState::getDataImpl(IODataCmn &data, size_t len)
{
  int raw;
  int ret;

  if (len != 1)
    {
      return -ENOTSUP;
    }

  ret = batRead(BATIOC_STATE, &raw);
  if (ret < 0)
    {
      return ret;
    }

  *reinterpret_cast<uint32_t *>(data.getDataPtr()) = static_cast<uint32_t>(raw);

  return OK;
}
