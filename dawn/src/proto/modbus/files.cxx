// dawn/src/proto/modbus/files.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/modbus/regs.hxx"

#include <algorithm>
#include <cstring>
#include <inttypes.h>
#include <new>

#include "dawn/io/common.hxx"
#include "dawn/io/viewdata.hxx"

using namespace dawn;

static constexpr uint32_t MODBUS_FILE_RECORDS = 10000;
static constexpr uint32_t MODBUS_FILE_BYTES = MODBUS_FILE_RECORDS * sizeof(uint16_t);
static constexpr uint32_t MODBUS_FILE_NUMBER_MAX = 65535;
static constexpr uint32_t MODBUS_SEEKABLE_WINDOW_DEFAULT = 8;

int CProtoModbusRegs::createFiles(const SProtoModbusIOBind *cfg)
{
  CIOCommon *io;
  SProtoModbusFile *file;
  uint64_t fileCount;
  uint64_t end;
  size_t i;

  // Each IO retains its window + 1 allocation. Consecutive files extend
  // the byte offset range in blocks of 10000 records.

  fileCount = cfg->config == 0 ? MODBUS_SEEKABLE_WINDOW_DEFAULT : cfg->config;
  fileCount += 1;
  end = cfg->start + fileCount * cfg->size;

  if (cfg->size == 0 || fileCount > MODBUS_FILE_NUMBER_MAX || end > MODBUS_FILE_NUMBER_MAX)
    {
      DAWNERR("seekable allocation exceeds Modbus file number range\n");
      return -EINVAL;
    }

  for (const auto *existing : files)
    {
      if (cfg->start + 1 < static_cast<uint32_t>(existing->start) + existing->fileCount &&
          existing->start < end + 1)
        {
          DAWNERR("overlapping seekable file allocations\n");
          return -EINVAL;
        }
    }

  for (i = 0; i < cfg->size; i++)
    {
      io = getIO_(cfg->objid[i]);
      if (io == nullptr || !io->isSeekable() || (!io->isRead() && !io->isWrite()) ||
          io->getDataSize() > fileCount * MODBUS_FILE_BYTES)
        {
          DAWNERR("invalid seekable IO 0x%08" PRIx32 "\n", cfg->objid[i]);
          return -EINVAL;
        }

      file = new (std::nothrow) SProtoModbusFile();
      if (file == nullptr)
        {
          DAWNERR("failed to allocate file handler\n");
          return -ENOMEM;
        }

      // Descriptor addresses are zero-based; Modbus file numbers start at 1.

      file->io = io;
      file->start = cfg->start + i * fileCount + 1;
      file->fileCount = fileCount;
      files.push_back(file);
    }

  return OK;
}

int CProtoModbusRegs::fileCb(uint8_t *buf,
                             uint16_t number,
                             uint16_t record,
                             uint16_t nregs,
                             enum nxmb_regmode_e mode)
{
  CIOCommon *io;
  size_t offset;
  size_t bytes;
  size_t size;
  bool read = (mode == NXMB_REG_READ);

  if (nregs == 0 || static_cast<uint32_t>(record) + nregs > MODBUS_FILE_RECORDS)
    {
      return -ENOENT;
    }

  for (auto *file : files)
    {
      if (number < file->start || number >= static_cast<uint32_t>(file->start) + file->fileCount)
        {
          continue;
        }

      std::lock_guard<std::mutex> lock(file->mutex);

      io = file->io;
      if ((read && !io->isRead()) || (!read && !io->isWrite()))
        {
          return -ENOENT;
        }

      offset = (number - file->start) * MODBUS_FILE_BYTES + record * sizeof(uint16_t);
      bytes = nregs * sizeof(uint16_t);

      if (read)
        {
          // Zero-fill the part of the requested window beyond the IO's end.

          std::memset(buf, 0, bytes);
          size = io->getDataSize();
          if (offset >= size)
            {
              return OK;
            }

          bytes = std::min(bytes, size - offset);
          io_data_view_t data(buf, bytes, bytes);

          return io->getData(data, 1, offset);
        }

      // Write complete records using the IO's existing offset semantics.

      io_data_view_t data(buf, bytes, bytes);

      return io->setData(data, offset);
    }

  return -ENOENT;
}
