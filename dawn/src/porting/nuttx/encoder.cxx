// dawn/src/porting/nuttx/encoder.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/encoder.hxx"

#include <errno.h>
#include <fcntl.h>
#include <nuttx/sensors/qencoder.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "dawn/debug.hxx"

int encoder_open(const char *path)
{
  int fd = open(path, O_RDWR);
  DAWNINFO("ENC: open %s %d\n", path, fd);
  if (fd < 0)
    {
      DAWNERR("Failed to open encoder file %s (error %d)\n", path, fd);
      return -EIO;
    }

  return fd;
}

void encoder_close(int fd)
{
  if (fd >= 0)
    {
      close(fd);
    }
}

int encoder_read_position(int fd, int32_t *pos)
{
  int ret = ioctl(fd, QEIOC_POSITION, reinterpret_cast<unsigned long>(pos));
  if (ret < 0)
    {
      DAWNERR("QEIOC_POSITION failed %d\n", -errno);
      return -errno;
    }

  return OK;
}

int encoder_read_index(int fd, dawn::porting::encoder_index_s *index)
{
  struct qe_index_s idx;
  int ret;

  ret = ioctl(fd, QEIOC_GETINDEX, reinterpret_cast<unsigned long>(&idx));
  if (ret < 0)
    {
      DAWNERR("QEIOC_GETINDEX failed %d\n", -errno);
      return -errno;
    }

  index->qenc_pos = idx.qenc_pos;
  index->indx_pos = idx.indx_pos;
  index->indx_cnt = idx.indx_cnt;

  return OK;
}

int encoder_reset(int fd)
{
  int ret = ioctl(fd, QEIOC_RESET, 0);
  if (ret < 0)
    {
      DAWNERR("QEIOC_RESET failed %d\n", -errno);
      return -errno;
    }

  return OK;
}

int encoder_set_posmax(int fd, uint32_t posmax)
{
  int ret = ioctl(fd, QEIOC_SETPOSMAX, static_cast<unsigned long>(posmax));
  if (ret < 0)
    {
      DAWNERR("QEIOC_SETPOSMAX failed %d\n", -errno);
      return -errno;
    }

  return OK;
}
