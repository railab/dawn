// dawn/apps/dawn/dawn_file_load.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn_file_load.hxx"

#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>

#include <cerrno>
#include <cstdio>

#include "dawn/common/descriptor.hxx"
#include "dawn/debug.hxx"
#include "dawn_file_load.hxx"

using namespace dawn;

int dawn_file_load_descriptor(const char *path, size_t maxSize, uint32_t **outBin, size_t *outLen)
{
  if (path == nullptr || outBin == nullptr || outLen == nullptr)
    {
      return -EINVAL;
    }

  // Open file

  int fd = open(path, O_RDONLY);
  if (fd < 0)
    {
      DAWNERR("ERROR: cannot open descriptor file '%s': %d\n", path, errno);
      return -ENOENT;
    }

  // Determine file size

  off_t fsize = lseek(fd, 0, SEEK_END);
  if (fsize < 0)
    {
      DAWNERR("ERROR: lseek failed on '%s': %d\n", path, errno);
      close(fd);
      return -errno;
    }

  if (static_cast<size_t>(fsize) > maxSize)
    {
      DAWNERR("ERROR: descriptor file '%s' too large (%ld > %zu)\n",
              path,
              static_cast<long>(fsize),
              maxSize);
      close(fd);
      return -EFBIG;
    }

  if (fsize < 8 || (fsize % sizeof(uint32_t)) != 0)
    {
      DAWNERR("ERROR: descriptor file '%s' has invalid size %ld\n", path, static_cast<long>(fsize));
      close(fd);
      return -EINVAL;
    }

  // Allocate buffer

  size_t bufsize = static_cast<size_t>(fsize);
  uint8_t *buf = new (std::nothrow) uint8_t[bufsize];
  if (buf == nullptr)
    {
      DAWNERR("ERROR: failed to allocate %zu bytes for descriptor\n", bufsize);
      close(fd);
      return -ENOMEM;
    }

  // Read entire file

  lseek(fd, 0, SEEK_SET);
  ssize_t nread = read(fd, buf, bufsize);
  close(fd);

  if (nread < 0 || static_cast<size_t>(nread) != bufsize)
    {
      DAWNERR("ERROR: short read on '%s': %zd / %zu\n", path, nread, bufsize);
      delete[] buf;
      return -EIO;
    }

  // Validate descriptor (magic, CRC, dtypes)

  uint32_t *bin = reinterpret_cast<uint32_t *>(buf);
  int ret = CDescriptor::binValid(bin, bufsize);
  if (ret < 0)
    {
      DAWNERR("ERROR: invalid descriptor file '%s': %d\n", path, ret);
      delete[] buf;
      return ret;
    }

  *outBin = bin;
  *outLen = bufsize;
  return 0;
}
