// dawn/src/io/fileio.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/fileio.hxx"

#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#include <cstring>

using namespace dawn;

/** @brief Allowed path prefixes for FileIO security whitelist. */

static const char g_prefix_data[] = "/data/";
static const char g_prefix_tmp[] = "/tmp/";
static const char g_prefix_dev[] = CONFIG_DAWN_IO_FILE_DEV_PREFIX;

// Device-node prefix is board-configurable; an empty one disables it.

static bool pathIsDev(const char *path)
{
  return sizeof(g_prefix_dev) > 1 &&
         std::strncmp(path, g_prefix_dev, sizeof(g_prefix_dev) - 1) == 0;
}

int CIOFile::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  size_t nbytes = 0;
  size_t i = 0;

  for (i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls == CIOCommon::IO_CLASS_ANY)
        {
          offset += cfgCmnOffset(item);
          continue;
        }

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_FILE)
        {
          DAWNERR("unsupported fileio cfg 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case IO_FILE_CFG_PATH:
            {
              nbytes = (size_t)item->cfgid.s.size * 4u;

              if (nbytes >= PATH_MAX)
                {
                  DAWNERR("file path too long: %zu bytes\n", nbytes);
                  return -ENAMETOOLONG;
                }

              std::memcpy(path, &item->data, nbytes);
              path[nbytes] = '\0';

              if (std::strstr(path, "..") != nullptr)
                {
                  DAWNERR("path traversal not allowed: %s\n", path);
                  return -EACCES;
                }

              if (std::strncmp(path, g_prefix_data, sizeof(g_prefix_data) - 1) != 0 &&
                  std::strncmp(path, g_prefix_tmp, sizeof(g_prefix_tmp) - 1) != 0 &&
                  !pathIsDev(path))
                {
                  DAWNERR("path not in allowed directory: %s\n", path);
                  return -EACCES;
                }

              offset += (1 + (size_t)item->cfgid.s.size);
              break;
            }

          case IO_FILE_CFG_PERM:
            {
              const uint8_t *tmp = reinterpret_cast<const uint8_t *>(&item->data);

              perm = *tmp;

              if (perm > IO_FILE_PERM_WRITE_ONCE)
                {
                  DAWNERR("invalid file permission: %u\n", (unsigned)perm);
                  return -EINVAL;
                }

              offset += 2;
              break;
            }

          default:
            {
              DAWNERR("unsupported fileio cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOFile::~CIOFile()
{
  deinit();
}

int CIOFile::configure()
{
  struct stat st;
  int flags;
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      return ret;
    }

  writeOnceLocked = false;

  // Select open flags based on permission

  if (perm == IO_FILE_PERM_READ)
    {
      flags = O_RDONLY;
    }
  else if (perm == IO_FILE_PERM_WRITE || perm == IO_FILE_PERM_WRITE_ONCE)
    {
      flags = O_WRONLY | O_CREAT;
    }
  else
    {
      // IO_FILE_PERM_RW

      flags = O_RDWR | O_CREAT;
    }

  fd = open(path, flags, 0666);
  if (fd < 0)
    {
      DAWNERR("failed to open %s: %d\n", path, errno);
      return -errno;
    }

  ret = fstat(fd, &st);
  if (ret < 0)
    {
      DAWNERR("fstat failed for %s: %d\n", path, errno);
      close(fd);
      fd = -1;
      return -errno;
    }

  // Under the device prefix only char or block device nodes are allowed

  devNode = S_ISCHR(st.st_mode) || S_ISBLK(st.st_mode);

  if (!devNode && pathIsDev(path))
    {
      DAWNERR("not a device node: %s\n", path);
      close(fd);
      fd = -1;
      return -EACCES;
    }

  // Device nodes report a zero st_size, so probe with SEEK_END; they have
  // a fixed size and cannot be truncated.

  fsize = (size_t)st.st_size;

  if (fsize == 0 && devNode)
    {
      off_t end = lseek(fd, 0, SEEK_END);
      if (end > 0)
        {
          fsize = (size_t)end;
        }

      lseek(fd, 0, SEEK_SET);
    }

  // A device node has a fixed size, so it cannot tell whether it was
  // written before - write-once is then enforced per session only.

  writeOnceLocked = perm == IO_FILE_PERM_WRITE_ONCE && !devNode && fsize > 0;

  // A write-only regular file starts empty from the IO point of view

  if (perm == IO_FILE_PERM_WRITE && !devNode)
    {
      fsize = 0;
    }

  return OK;
}

int CIOFile::truncateFile()
{
  int ret;

  // Device nodes have no truncate operation

  if (devNode)
    {
      return OK;
    }

  ret = ftruncate(fd, 0);
  if (ret < 0)
    {
      DAWNERR("ftruncate failed: %d\n", errno);
      return -EIO;
    }

  return OK;
}

int CIOFile::deinit()
{
  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }

  return OK;
}

int CIOFile::getDataImpl(IODataCmn &data, size_t len)
{
  ssize_t nread;

  if (len != 1)
    {
      return -EINVAL;
    }

  if (data.getDataSize() < fsize)
    {
      return -ENOMEM;
    }

  if (lseek(fd, 0, SEEK_SET) < 0)
    {
      DAWNERR("lseek failed: %d\n", errno);
      return -EIO;
    }

  nread = read(fd, data.getDataPtr(0), fsize);
  if (nread < 0)
    {
      DAWNERR("read failed: %d\n", errno);
      return -EIO;
    }

  return OK;
}

int CIOFile::setDataImpl(IODataCmn &data)
{
  ssize_t nwritten;
  int ret;

  if (perm == IO_FILE_PERM_WRITE_ONCE && writeOnceLocked)
    {
      DAWNERR("write-once file already written: %s\n", path);
      return -EPERM;
    }

  ret = truncateFile();
  if (ret < 0)
    {
      return ret;
    }

  if (lseek(fd, 0, SEEK_SET) < 0)
    {
      DAWNERR("lseek failed: %d\n", errno);
      return -EIO;
    }

  nwritten = write(fd, data.getDataPtr(0), data.getDataSize());
  if (nwritten < 0)
    {
      DAWNERR("write failed: %d\n", errno);
      return -EIO;
    }

  // A device node keeps its fixed size

  if (!devNode)
    {
      fsize = (size_t)nwritten;
    }

  if (perm == IO_FILE_PERM_WRITE_ONCE)
    {
      writeOnceLocked = true;
    }

  return OK;
}

int CIOFile::getDataAtImpl(IODataCmn &data, size_t len, size_t offset)
{
  ssize_t nread;

  if (len != 1)
    {
      return -EINVAL;
    }

  if (offset >= fsize)
    {
      return -EINVAL;
    }

  if (lseek(fd, (off_t)offset, SEEK_SET) < 0)
    {
      DAWNERR("lseek failed: %d\n", errno);
      return -EIO;
    }

  nread = read(fd, data.getDataPtr(0), data.getDataSize());
  if (nread < 0)
    {
      DAWNERR("read failed: %d\n", errno);
      return -EIO;
    }

  return OK;
}

int CIOFile::setDataAtImpl(IODataCmn &data, size_t offset)
{
  ssize_t nwritten;
  int ret;
  size_t endpos;

  if (perm == IO_FILE_PERM_WRITE_ONCE && writeOnceLocked)
    {
      DAWNERR("write-once file already written: %s\n", path);
      return -EPERM;
    }

  if (offset == 0)
    {
      ret = truncateFile();
      if (ret < 0)
        {
          return ret;
        }
    }

  if (lseek(fd, (off_t)offset, SEEK_SET) < 0)
    {
      DAWNERR("lseek failed: %d\n", errno);
      return -EIO;
    }

  nwritten = write(fd, data.getDataPtr(0), data.getDataSize());
  if (nwritten < 0)
    {
      DAWNERR("write failed: %d\n", errno);
      return -EIO;
    }

  endpos = offset + (size_t)nwritten;
  if (!devNode && (offset == 0 || endpos > fsize))
    {
      fsize = endpos;
    }

  if (perm == IO_FILE_PERM_WRITE_ONCE)
    {
      writeOnceLocked = true;
    }

  return OK;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CIOFile::getFd() const
{
  return -1;
}
#endif

size_t CIOFile::getDataSize() const
{
  return fsize;
}

size_t CIOFile::getDataDim() const
{
  // Block payload uses 1-byte data units.

  return fsize;
}

bool CIOFile::isRead() const
{
  return perm == IO_FILE_PERM_READ || perm == IO_FILE_PERM_RW;
}

bool CIOFile::isWrite() const
{
  return perm == IO_FILE_PERM_WRITE || perm == IO_FILE_PERM_RW || perm == IO_FILE_PERM_WRITE_ONCE;
}
