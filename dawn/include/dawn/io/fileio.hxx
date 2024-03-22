// dawn/include/dawn/io/fileio.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <limits.h>

#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief File system I/O access.
 *
 * Provides read/write access to files on the device file system using the
 * Dawn IO abstraction. Access is restricted to the /data/ and /tmp/
 * directories to prevent path-injection attacks via a crafted descriptor.
 *
 * The file path is read-only configuration hardcoded in the device
 * descriptor and cannot be changed at runtime.
 */

class CIOFile : public CIOCommon
{
public:
  enum
  {
    IO_FILE_PERM_READ = 0,       ///< Read-only
    IO_FILE_PERM_WRITE = 1,      ///< Write-only
    IO_FILE_PERM_RW = 2,         ///< Read-write
    IO_FILE_PERM_WRITE_ONCE = 3, ///< Write-once
  };

  enum
  {
    IO_FILE_CFG_FIRST = 0,
    IO_FILE_CFG_PATH = 1, ///< File path (null-terminated string, DTYPE_CHAR).
    IO_FILE_CFG_PERM = 2, ///< Permission mode (EIOFilePerm, DTYPE_UINT8).
    IO_FILE_CFG_LAST = 31
  };

  explicit CIOFile(CDescObject &desc)
    : CIOCommon(desc)
    , fd(-1)
    , fsize(0)
    , perm(IO_FILE_PERM_READ)
    , writeOnceLocked(false)
  {
  }

  ~CIOFile() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "file";
  }
#endif

  int configure() override;
  int deinit() override;
  int getDataImpl(IODataCmn &data, size_t len) override;
  int setDataImpl(IODataCmn &data) override;
  int getDataAtImpl(IODataCmn &data, size_t len, size_t offset) override;
  int setDataAtImpl(IODataCmn &data, size_t offset) override;

#ifdef CONFIG_DAWN_IO_NOTIFY
  int getFd() const override;
#endif

  size_t getDataSize() const override;
  size_t getDataDim() const override;
  bool isRead() const override;
  bool isWrite() const override;

  bool isNotify() const override
  {
    return false;
  }

  bool isBatch() const override
  {
    return false;
  }

  bool isSeekable() const override
  {
    return true;
  }

  using ObjectIdHelper = CIOCommon::IOObjectIdHelperNoTS<IO_CLASS_FILE, SObjectId::DTYPE_BLOCK>;

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return ObjectIdHelper::create(inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPath(uint16_t size)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_FILE,
                                 SObjectId::DTYPE_CHAR,
                                 false,
                                 size,
                                 IO_FILE_CFG_PATH);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdPerm()
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_IO,
                                 CIOCommon::IO_CLASS_FILE,
                                 SObjectId::DTYPE_UINT8,
                                 false,
                                 1,
                                 IO_FILE_CFG_PERM);
  }

private:
  char path[PATH_MAX] = {}; ///< Null-terminated file path copied from descriptor.
  int fd;                   ///< File descriptor; -1 when not open.
  size_t fsize;             ///< Tracked file size in bytes (updated after successful writes).
  uint8_t perm;             ///< Configured permission mode.
  bool writeOnceLocked;     ///< Whether WRITE_ONCE has already consumed its single write.

  int configureDesc(const CDescObject &desc);
};
} // Namespace dawn
