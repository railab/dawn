// dawn/src/proto/modbus/regs.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/modbus/regs.hxx"

#include <cstring>
#include <inttypes.h>
#include <new>

#include "dawn/io/common.hxx"
#include "dawn/io/viewdata.hxx"

using namespace dawn;

static inline void modbus_set_bit(uint8_t *buf, uint16_t bit, uint8_t value)
{
  uint8_t *byte = &buf[bit / 8];
  uint8_t mask = (uint8_t)(1u << (bit % 8));

  if (value)
    {
      *byte |= mask;
    }
  else
    {
      *byte &= (uint8_t)~mask;
    }
}

static inline uint8_t modbus_get_bit(const uint8_t *buf, uint16_t bit)
{
  return (uint8_t)((buf[bit / 8] >> (bit % 8)) & 0x1u);
}

static inline size_t modbus_seekable_window_regs(const CProtoModbusRegs::SProtoModbusRegs *reg)
{
  static constexpr size_t default_window = 8;
  return reg->cfg->config > 0 ? reg->cfg->config : default_window;
}

#ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
CProtoModbusRegs::SSeekableState *CProtoModbusRegs::findSeekableState(SProtoModbusRegs *reg)
{
  for (auto *state : vseekable)
    {
      if (state->reg == reg)
        {
          return state;
        }
    }

  return nullptr;
}
#endif

CProtoModbusRegs::SProtoModbusRegs *
CProtoModbusRegs::findGroup(uint16_t addr, uint16_t n, const std::vector<SProtoModbusRegs *> &regs)
{
  SProtoModbusRegs *reg = nullptr;

  // Search from the end so later descriptor groups override earlier ones
  // when address ranges overlap.

  for (auto it = regs.rbegin(); it != regs.rend(); ++it)
    {
      auto *v = *it;

      if (v == nullptr || v->cfg == nullptr)
        {
          continue;
        }

      // Found

      if ((addr >= v->cfg->start) && ((addr + n) <= (v->cfg->start + v->regs)))
        {
          reg = v;
          break;
        }
    }

  return reg;
}

const CProtoModbusRegs::SProtoModbusRegs *CProtoModbusRegs::hasOverlap(
  const std::vector<SProtoModbusRegs *> &regs,
  uint32_t start,
  size_t count) const
{
  uint32_t end = start + static_cast<uint32_t>(count);

  for (auto const *existing : regs)
    {
      if (existing == nullptr || existing->cfg == nullptr)
        {
          continue;
        }

      uint32_t ex_start = existing->cfg->start;
      uint32_t ex_end = ex_start + static_cast<uint32_t>(existing->regs);

      if (start < ex_end && ex_start < end)
        {
          return existing;
        }
    }

  return nullptr;
}

void CProtoModbusRegs::cleanupGroups(std::vector<SProtoModbusRegs *> &groups)
{
  for (auto *v : groups)
    {
      delete[] reinterpret_cast<uint8_t *>(v->data);

      for (auto *b : v->iodata)
        {
          delete b;
        }

      v->iodata.clear();
      delete v;
    }

  groups.clear();
}

static size_t getIoRegBytes(const CProtoModbusRegs::SProtoModbusRegs *reg, size_t index)
{
  if (reg->cfg->type == CProtoModbusRegs::MODBUS_TYPE_SEEKABLE)
    {
      return (modbus_seekable_window_regs(reg) + 1) * sizeof(uint16_t);
    }

  return reg->iodata[index]->getDataSize();
}

// Map a byte/bit offset in Modbus register space back to the bound IO and
// offset inside that IO. Packed coil/discrete groups can expose one vector IO
// as multiple Modbus bits, so the Modbus index is not always the IO index.

static int findIoIndexByByteOffset(const CProtoModbusRegs::SProtoModbusRegs *reg,
                                   int index,
                                   size_t *offsetInIo)
{
  size_t regOffset = 0;

  for (size_t j = 0; j < reg->iodata.size(); j++)
    {
      size_t bytes = getIoRegBytes(reg, j);

      if (index >= static_cast<int>(regOffset) && index < static_cast<int>(regOffset + bytes))
        {
          *offsetInIo = static_cast<size_t>(index) - regOffset;
          return static_cast<int>(j);
        }

      regOffset += bytes;
    }

  return -1;
}

#if defined(CONFIG_DAWN_PROTO_MODBUS_COIL) || defined(CONFIG_DAWN_PROTO_MODBUS_DISCRETE)
int CProtoModbusRegs::regReadWriteCoil(uint8_t *buff,
                                       uint16_t n,
                                       int index,
                                       bool read,
                                       SProtoModbusRegs *reg)
{
  io_ddata_t *iodata = nullptr;
  CIOCommon *io = nullptr;
  uint8_t *ptr = nullptr;
  int err = 0;
  int ret;
  size_t size;

  reg->mutex.lock();

  while (n > 0)
    {
      if (index >= reg->ios)
        {
          err = -EIO;
          break;
        }

      io = getIO_(reg->cfg->objid[index]);
      iodata = reg->iodata[index];
      ptr = reinterpret_cast<uint8_t *>(iodata->getDataPtr());
      size = iodata->getDataSize();

      if (size > n)
        {
          err = -EINVAL;
          break;
        }

      if (read)
        {
          if (io->isRead())
            {
              ret = io->getData(*iodata, 1);
              if (ret != OK)
                {
                  err = -EIO;
                  break;
                }
            }

          for (size_t i = 0; i < size; i++)
            {
              *buff++ = ptr[i];
            }
        }
      else
        {
          for (size_t i = 0; i < size; i++)
            {
              ptr[i] = *buff++;
            }

          ret = io->setData(*iodata);
          if (ret != OK)
            {
              err = -EIO;
              break;
            }
        }

      index += static_cast<int>(size);
      n -= static_cast<uint16_t>(size);
    }

  reg->mutex.unlock();
  return err;
}

int CProtoModbusRegs::regReadWriteCoilPacked(uint8_t *buff,
                                             uint16_t n,
                                             int index,
                                             bool read,
                                             SProtoModbusRegs *reg)
{
  io_ddata_t *iodata = nullptr;
  CIOCommon *io = nullptr;
  uint8_t *ptr = nullptr;
  int err = 0;
  int ret;

  reg->mutex.lock();

  if (read)
    {
      memset(buff, 0, (n + 7) / 8);
    }

  for (uint16_t bit = 0; bit < n; bit++)
    {
      size_t offsetInIo = 0;
      const int io_index = findIoIndexByByteOffset(reg, index + bit, &offsetInIo);

      if (io_index < 0 || io_index >= reg->ios)
        {
          if (read)
            {
              continue;
            }

          err = -EIO;
          break;
        }

      io = getIO_(reg->cfg->objid[io_index]);
      iodata = reg->iodata[io_index];
      ptr = reinterpret_cast<uint8_t *>(iodata->getDataPtr());
      size_t size = iodata->getDataSize();

      if (offsetInIo >= size)
        {
          err = -EINVAL;
          break;
        }

      if (read)
        {
          if (io->isRead())
            {
              ret = io->getData(*iodata, 1);
              if (ret != OK)
                {
                  err = -EIO;
                  break;
                }
            }

          modbus_set_bit(buff, bit, (ptr[offsetInIo] == 0) ? 0 : 1);
        }
      else
        {
          if (io->isRead())
            {
              ret = io->getData(*iodata, 1);
              if (ret != OK)
                {
                  err = -EIO;
                  break;
                }
            }

          ptr[offsetInIo] = modbus_get_bit(buff, bit) ? 1 : 0;

          ret = io->setData(*iodata);
          if (ret != OK)
            {
              err = -EIO;
              break;
            }
        }
    }

  reg->mutex.unlock();
  return err;
}
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
int CProtoModbusRegs::seekableRegRW(uint8_t *buff,
                                    uint16_t n,
                                    int index,
                                    bool read,
                                    SProtoModbusRegs *reg,
                                    SSeekableState *seekState)
{
  io_ddata_t *iodata = nullptr;
  CIOCommon *io = nullptr;
  uint8_t *ptr = nullptr;
  uint16_t value = 0;
  int err = 0;
  int ret;
  size_t i;

  size_t windowRegs = seekState->windowRegs;
  size_t regsPerIo = windowRegs + 1;

  while (n > 0)
    {
      if (index < 0)
        {
          err = -EINVAL;
          break;
        }

      size_t ioIndex = static_cast<size_t>(index) / regsPerIo;
      size_t inIo = static_cast<size_t>(index) % regsPerIo;
      if (ioIndex >= static_cast<size_t>(reg->ios))
        {
          err = -EIO;
          break;
        }

      // Seek register access

      if (inIo == windowRegs)
        {
          if (read)
            {
              seekState->readProgressWords[ioIndex] = 0;
              value = static_cast<uint16_t>(seekState->seekOffsets[ioIndex]);
              *buff++ = static_cast<uint8_t>((value >> 8) & 0xff);
              *buff++ = static_cast<uint8_t>(value & 0xff);
            }
          else
            {
              seekState->seekOffsets[ioIndex] =
                static_cast<size_t>(((uint16_t)buff[0] << 8) | (uint16_t)buff[1]);
              seekState->readProgressWords[ioIndex] = 0;
              buff += sizeof(uint16_t);
            }

          index += 1;
          n -= 1;
          continue;
        }

      // Data window access

      io = getIO_(reg->cfg->objid[ioIndex]);
      if (!read && !io->isWrite())
        {
          err = -EINVAL;
          break;
        }

      size_t accessWords = windowRegs - inIo;
      if (accessWords > n)
        {
          accessWords = n;
        }

      iodata = reg->iodata[ioIndex];
      ptr = reinterpret_cast<uint8_t *>(iodata->getDataPtr());
      size_t accessBytes = accessWords * sizeof(uint16_t);
      io_data_view_t windowData(ptr, accessBytes, accessBytes);
      std::memset(ptr, 0, accessBytes);

      size_t byteOffset = seekState->seekOffsets[ioIndex] + (inIo * sizeof(uint16_t));

      if (read)
        {
          size_t totalBytes = io->getDataSize();
          if (byteOffset < totalBytes)
            {
              ret = io->getData(windowData, 1, byteOffset);
              if (ret != OK)
                {
                  err = -EIO;
                  break;
                }
            }

          for (i = 0; i < accessWords; i++)
            {
              value =
                static_cast<uint16_t>(ptr[i * 2]) | (static_cast<uint16_t>(ptr[i * 2 + 1]) << 8);
              *buff++ = static_cast<uint8_t>((value >> 8) & 0xff);
              *buff++ = static_cast<uint8_t>(value & 0xff);
            }

          size_t progressedWords = seekState->readProgressWords[ioIndex];
          if (inIo == 0)
            {
              progressedWords = accessWords;
            }
          else if (progressedWords == inIo)
            {
              progressedWords += accessWords;
            }
          else
            {
              progressedWords = 0;
            }

          if (progressedWords >= windowRegs)
            {
              seekState->seekOffsets[ioIndex] += windowRegs * sizeof(uint16_t);
              progressedWords = 0;
            }

          seekState->readProgressWords[ioIndex] = progressedWords;
        }
      else
        {
          for (i = 0; i < accessWords; i++)
            {
              ptr[i * 2] = buff[1];
              ptr[i * 2 + 1] = buff[0];
              buff += sizeof(uint16_t);
            }

          ret = io->setData(windowData, byteOffset);
          if (ret != OK)
            {
              err = -EIO;
              break;
            }

          seekState->readProgressWords[ioIndex] = 0;
        }

      index += static_cast<int>(accessWords);
      n -= static_cast<uint16_t>(accessWords);
    }

  return err;
}
#endif

#if defined(CONFIG_DAWN_PROTO_MODBUS_INPUT) || defined(CONFIG_DAWN_PROTO_MODBUS_HOLDING)
static int findIoIndexByRegOffset(const CProtoModbusRegs::SProtoModbusRegs *reg, int index)
{
  size_t regOffset = 0;
  for (size_t j = 0; j < reg->iodata.size(); j++)
    {
      size_t words = getIoRegBytes(reg, j) / sizeof(uint16_t);

      if (index == static_cast<int>(regOffset))
        {
          return static_cast<int>(j);
        }

      regOffset += words;
    }

  return -1;
}

int CProtoModbusRegs::standardRegRW(uint8_t *buff,
                                    uint16_t n,
                                    int index,
                                    bool read,
                                    SProtoModbusRegs *reg)
{
  io_ddata_t *iodata = nullptr;
  CIOCommon *io = nullptr;
  uint8_t *ptr = nullptr;
  uint16_t value = 0;
  int err = 0;
  int ret;
  size_t words;
  size_t i;

  while (n > 0)
    {
      int ioIndex = findIoIndexByRegOffset(reg, index);
      if (ioIndex < 0 || ioIndex >= reg->ios)
        {
          err = -EIO;
          break;
        }

      io = getIO_(reg->cfg->objid[ioIndex]);
      iodata = reg->iodata[ioIndex];
      ptr = reinterpret_cast<uint8_t *>(iodata->getDataPtr());
      words = iodata->getDataSize() / sizeof(uint16_t);

      if (words > n)
        {
          err = -EINVAL;
          break;
        }

      if (read)
        {
          if (io->isRead())
            {
              ret = io->getData(*iodata, 1);
              if (ret != OK)
                {
                  err = -EIO;
                  break;
                }
            }

          for (i = 0; i < words; i++)
            {
              value =
                static_cast<uint16_t>(ptr[i * 2]) | (static_cast<uint16_t>(ptr[i * 2 + 1]) << 8);
              *buff++ = static_cast<uint8_t>((value >> 8) & 0xff);
              *buff++ = static_cast<uint8_t>(value & 0xff);
            }
        }
      else
        {
          for (i = 0; i < words; i++)
            {
              ptr[i * 2] = buff[1];
              ptr[i * 2 + 1] = buff[0];
              buff += sizeof(uint16_t);
            }

          ret = io->setData(*iodata);
          if (ret != OK)
            {
              err = -EIO;
              break;
            }
        }

      index += static_cast<int>(words);
      n -= static_cast<uint16_t>(words);
    }

  return err;
}

int CProtoModbusRegs::regReadWriteHolding(uint8_t *buff,
                                          uint16_t n,
                                          int index,
                                          bool read,
                                          SProtoModbusRegs *reg)
{
  reg->mutex.lock();
  int err;

#  ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
  if (reg->cfg->type == MODBUS_TYPE_SEEKABLE)
    {
      SSeekableState *seekState = findSeekableState(reg);
      err = (seekState != nullptr) ? seekableRegRW(buff, n, index, read, reg, seekState) : -EIO;
    }
  else
#  endif
    {
      err = standardRegRW(buff, n, index, read, reg);
    }

  reg->mutex.unlock();
  return err;
}
#endif

CProtoModbusRegs::~CProtoModbusRegs()
{
}

void CProtoModbusRegs::valloc_push_back(SProtoModbusIOBind *alloc)
{
  valloc.push_back(alloc);
}

#ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
static int setupSeekableRegs(CProtoModbusRegs::SProtoModbusRegs *tmp,
                             size_t *seekWindowRegs,
                             bool *addSeekableState)
{
  *seekWindowRegs = modbus_seekable_window_regs(tmp);
  if (*seekWindowRegs == 0)
    {
      DAWNERR("invalid seekable window size\n");
      return -EINVAL;
    }

  *addSeekableState = true;
  return OK;
}
#else
static int setupSeekableRegs(CProtoModbusRegs::SProtoModbusRegs *tmp,
                             size_t *seekWindowRegs,
                             bool *addSeekableState)
{
  (void)tmp;
  *seekWindowRegs = 0;
  *addSeekableState = false;
  DAWNERR("seekable register type disabled at compile time\n");
  return -ENOTSUP;
}
#endif

static int validateRegisterIo(CIOCommon *io, uint32_t type, size_t seekWindowRegs)
{
  switch (type)
    {
#ifdef CONFIG_DAWN_PROTO_MODBUS_COIL
      case CProtoModbusRegs::MODBUS_TYPE_COIL:
        {
          if (io->isSeekable())
            {
              DAWNERR("seekable IO not supported for coil regs\n");
              return -ENOTSUP;
            }

          if (io->isWrite() == false)
            {
              DAWNERR("coil reg doesn't support write\n");
              return -EINVAL;
            }

          if (io->getDataSize() == 0 || io->getDataDim() == 0)
            {
              DAWNERR("unsupported IO type for coil\n");
              return -EINVAL;
            }

          return static_cast<int>(io->getDataSize());
        }

      case CProtoModbusRegs::MODBUS_TYPE_COIL_PACKED:
        {
          if (io->isSeekable())
            {
              DAWNERR("seekable IO not supported for coil regs\n");
              return -ENOTSUP;
            }

          if (io->isWrite() == false)
            {
              DAWNERR("coil reg doesn't support write\n");
              return -EINVAL;
            }

          if (io->getDataSize() == 0 || io->getDataDim() == 0)
            {
              DAWNERR("unsupported IO type for packed coil\n");
              return -EINVAL;
            }

          return static_cast<int>(io->getDataSize());
        }
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_DISCRETE
      case CProtoModbusRegs::MODBUS_TYPE_DISCRETE:
        {
          if (io->isSeekable())
            {
              DAWNERR("seekable IO not supported for discrete regs\n");
              return -ENOTSUP;
            }

          if (io->isRead() == false)
            {
              DAWNERR("discrete reg doesn't support read\n");
              return -EINVAL;
            }

          if (io->getDataSize() == 0 || io->getDataDim() == 0)
            {
              DAWNERR("unsupported IO type for discrete\n");
              return -EINVAL;
            }

          return static_cast<int>(io->getDataSize());
        }

      case CProtoModbusRegs::MODBUS_TYPE_DISCRETE_PACKED:
        {
          if (io->isSeekable())
            {
              DAWNERR("seekable IO not supported for discrete regs\n");
              return -ENOTSUP;
            }

          if (io->isRead() == false)
            {
              DAWNERR("discrete reg doesn't support read\n");
              return -EINVAL;
            }

          if (io->getDataSize() == 0 || io->getDataDim() == 0)
            {
              DAWNERR("unsupported IO type for packed discrete\n");
              return -EINVAL;
            }

          return static_cast<int>(io->getDataSize());
        }
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_INPUT
      case CProtoModbusRegs::MODBUS_TYPE_INPUT:
        {
          if (io->isSeekable())
            {
              DAWNERR("seekable IO not supported for input regs\n");
              return -ENOTSUP;
            }

          if (io->isRead() == false)
            {
              DAWNERR("input reg doesn't support read\n");
              return -EINVAL;
            }

          size_t bytes = io->getDataSize();

          if (bytes == 0 || bytes % sizeof(uint16_t) != 0)
            {
              DAWNERR("unsupported IO type for input\n");
              return -EINVAL;
            }

          return static_cast<int>(bytes);
        }
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_HOLDING
      case CProtoModbusRegs::MODBUS_TYPE_HOLDING:
        {
          if (io->isSeekable())
            {
              DAWNERR("seekable IO requires seekable reg type\n");
              return -ENOTSUP;
            }

          if (!io->isWrite())
            {
              DAWNERR("holding reg doesn't support write\n");
              return -EINVAL;
            }

          size_t bytes = io->getDataSize();

          if (bytes == 0 || bytes % sizeof(uint16_t) != 0)
            {
              DAWNERR("unsupported IO type for holding\n");
              return -EINVAL;
            }

          return static_cast<int>(bytes);
        }
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
      case CProtoModbusRegs::MODBUS_TYPE_SEEKABLE:
        {
          if (!io->isSeekable())
            {
              DAWNERR("seekable reg requires seekable IO\n");
              return -EINVAL;
            }

          if (!io->isRead())
            {
              DAWNERR("seekable reg doesn't support read\n");
              return -EINVAL;
            }

          return static_cast<int>((seekWindowRegs + 1) * sizeof(uint16_t));
        }
#endif

      default:
        {
          DAWNERR("Unsupported Modbus reg type %" PRIu32 "\n", type);
          return -EINVAL;
        }
    }
}

int CProtoModbusRegs::finalizeRegGroup(SProtoModbusRegs *tmp,
                                       const SProtoModbusIOBind *v,
                                       size_t seekWindowRegs,
                                       bool addSeekableState)
{
  switch (v->type)
    {
#ifdef CONFIG_DAWN_PROTO_MODBUS_COIL
      case MODBUS_TYPE_COIL:
      case MODBUS_TYPE_COIL_PACKED:
        {
          tmp->regs = tmp->size / sizeof(uint8_t);
          auto *overlap = hasOverlap(vcoils, v->start, tmp->regs);
          if (overlap != nullptr)
            {
              DAWNERR("overlapping coil ranges: start=%" PRIu32 " count=%zu overlaps start=%" PRIu32
                      " count=%zu\n",
                      v->start,
                      tmp->regs,
                      overlap->cfg->start,
                      overlap->regs);
              return -EINVAL;
            }

          vcoils.push_back(tmp);
          break;
        }
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_DISCRETE
      case MODBUS_TYPE_DISCRETE:
      case MODBUS_TYPE_DISCRETE_PACKED:
        {
          tmp->regs = tmp->size / sizeof(uint8_t);
          auto *overlap = hasOverlap(vdiscrete, v->start, tmp->regs);
          if (overlap != nullptr)
            {
              DAWNERR("overlapping discrete ranges: start=%" PRIu32
                      " count=%zu overlaps start=%" PRIu32 " count=%zu\n",
                      v->start,
                      tmp->regs,
                      overlap->cfg->start,
                      overlap->regs);
              return -EINVAL;
            }

          vdiscrete.push_back(tmp);
          break;
        }
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_INPUT
      case MODBUS_TYPE_INPUT:
        {
          tmp->regs = tmp->size / sizeof(uint16_t);
          auto *overlap = hasOverlap(vinput, v->start, tmp->regs);
          if (overlap != nullptr)
            {
              DAWNERR("overlapping input ranges: start=%" PRIu32
                      " count=%zu overlaps start=%" PRIu32 " count=%zu\n",
                      v->start,
                      tmp->regs,
                      overlap->cfg->start,
                      overlap->regs);
              return -EINVAL;
            }

          vinput.push_back(tmp);
          break;
        }
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_HOLDING
      case MODBUS_TYPE_HOLDING:
      case MODBUS_TYPE_SEEKABLE:
        {
          tmp->regs = tmp->size / sizeof(uint16_t);
          auto *overlap = hasOverlap(vholding, v->start, tmp->regs);

          if (overlap != nullptr)
            {
              DAWNERR("overlapping holding ranges: start=%" PRIu32
                      " count=%zu overlaps start=%" PRIu32 " count=%zu\n",
                      v->start,
                      tmp->regs,
                      overlap->cfg->start,
                      overlap->regs);
              return -EINVAL;
            }

          vholding.push_back(tmp);

          if (addSeekableState)
            {
#  ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
              SSeekableState *state = new (std::nothrow) SSeekableState();
              if (state == nullptr)
                {
                  DAWNERR("failed to allocate seekable state\n");
                  return -ENOMEM;
                }

              state->reg = tmp;
              state->windowRegs = seekWindowRegs;
              state->seekOffsets.assign(v->size, 0);
              state->readProgressWords.assign(v->size, 0);
              vseekable.push_back(state);
#  endif
            }

          break;
        }
#endif

      default:
        {
          DAWNERR("Unsupported Modbus reg type %" PRIu32 "\n", v->type);
          return -EINVAL;
        }
    }

  return OK;
}

int CProtoModbusRegs::createRegs()
{
  if (initialized)
    {
      return OK;
    }

  for (auto const *v : valloc)
    {
      SProtoModbusRegs *tmp;
      size_t seekWindowRegs = 0;
      bool addSeekableState = false;

      tmp = new (std::nothrow) SProtoModbusRegs();
      if (!tmp)
        {
          DAWNERR("failed to allocate register handler\n");
          return -ENOMEM;
        }

      tmp->cfg = v;
      tmp->data = nullptr;
      tmp->regs = 0;
      tmp->size = 0;
      tmp->ios = 0;
      tmp->iodata.reserve(v->size);

      if (v->type == MODBUS_TYPE_SEEKABLE)
        {
          int ret = setupSeekableRegs(tmp, &seekWindowRegs, &addSeekableState);
          if (ret != OK)
            {
              return ret;
            }
        }

      for (size_t i = 0; i < v->size; i++)
        {
          CIOCommon *io;
          io_ddata_t *iobuffer;

          io = getIO_(v->objid[i]);
          if (io == nullptr)
            {
              DAWNERR("Failed to get IO 0x%08" PRIx32 "\n", v->objid[i]);
              return -EIO;
            }

          int sizeInc = validateRegisterIo(io, v->type, seekWindowRegs);
          if (sizeInc < 0)
            {
              return sizeInc;
            }

          tmp->size += static_cast<size_t>(sizeInc);
          tmp->ios += 1;

          if (v->type == MODBUS_TYPE_SEEKABLE)
            {
              iobuffer = io->ddata_alloc(1, seekWindowRegs * sizeof(uint16_t));
            }
          else
            {
              iobuffer = io->ddata_alloc(1);
            }

          if (!iobuffer)
            {
              DAWNERR("failed to allocate register buffer\n");
              return -ENOMEM;
            }

          tmp->iodata.push_back(iobuffer);
        }

      int ret = finalizeRegGroup(tmp, v, seekWindowRegs, addSeekableState);
      if (ret != OK)
        {
          return ret;
        }
    }

  initialized = true;
  return OK;
}

int CProtoModbusRegs::destroyRegs()
{
  if (!initialized)
    {
      return OK;
    }

#ifdef CONFIG_DAWN_PROTO_MODBUS_COIL
  cleanupGroups(vcoils);
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_DISCRETE
  cleanupGroups(vdiscrete);
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_INPUT
  cleanupGroups(vinput);
#endif
#ifdef CONFIG_DAWN_PROTO_MODBUS_HOLDING
  cleanupGroups(vholding);
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_SEEKABLE
  for (auto *state : vseekable)
    {
      delete state;
    }

  vseekable.clear();
#endif

  valloc.clear();
  initialized = false;

  return OK;
}

#ifdef CONFIG_DAWN_PROTO_MODBUS_COIL
int CProtoModbusRegs::coilsCb(uint8_t *buf,
                              uint16_t addr,
                              uint16_t ncoils,
                              enum nxmb_regmode_e mode,
                              void *priv)
{
  SProtoModbusRegs *reg = nullptr;
  int index;
  int ret_helper;
  bool read = (mode == NXMB_REG_READ);

  (void)priv;

  reg = findGroup(addr, ncoils, vcoils);
  if (!reg)
    {
      return -ENOENT;
    }

  index = (int)(addr - reg->cfg->start);

  DAWNINFO("coilsCb %d %d %d\n", addr, ncoils, index);

  if (reg->cfg->type == MODBUS_TYPE_COIL_PACKED)
    {
      ret_helper = regReadWriteCoilPacked(buf, ncoils, index, read, reg);
    }
  else
    {
      ret_helper = regReadWriteCoil(buf, ncoils, index, read, reg);
    }

  return ret_helper;
}
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_DISCRETE
int CProtoModbusRegs::discreteCb(uint8_t *buf, uint16_t addr, uint16_t ndiscrete, void *priv)
{
  SProtoModbusRegs *reg = nullptr;
  int index;
  int ret_helper;

  (void)priv;

  reg = findGroup(addr, ndiscrete, vdiscrete);
  if (!reg)
    {
      return -ENOENT;
    }

  index = (int)(addr - reg->cfg->start);

  DAWNINFO("discreteCb %d %d %d\n", addr, ndiscrete, index);

  if (reg->cfg->type == MODBUS_TYPE_DISCRETE_PACKED)
    {
      ret_helper = regReadWriteCoilPacked(buf, ndiscrete, index, true, reg);
    }
  else
    {
      ret_helper = regReadWriteCoil(buf, ndiscrete, index, true, reg);
    }

  return ret_helper;
}
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_INPUT
int CProtoModbusRegs::inputCb(uint8_t *buf, uint16_t addr, uint16_t nregs, void *priv)
{
  SProtoModbusRegs *reg = nullptr;
  int index;
  int ret_helper;

  (void)priv;

  reg = findGroup(addr, nregs, vinput);
  if (!reg)
    {
      return -ENOENT;
    }

  index = (int)(addr - reg->cfg->start);

  DAWNINFO("inputCb %d %d %d\n", addr, nregs, index);

  ret_helper = regReadWriteHolding(buf, nregs, index, true, reg);

  return ret_helper;
}
#endif

#ifdef CONFIG_DAWN_PROTO_MODBUS_HOLDING
int CProtoModbusRegs::holdingCb(uint8_t *buf,
                                uint16_t addr,
                                uint16_t nregs,
                                enum nxmb_regmode_e mode,
                                void *priv)
{
  SProtoModbusRegs *reg = nullptr;
  int index;
  int ret_helper;
  bool read = (mode == NXMB_REG_READ);

  (void)priv;

  reg = findGroup(addr, nregs, vholding);
  if (!reg)
    {
      return -ENOENT;
    }

  index = (int)(addr - reg->cfg->start);

  DAWNINFO("holdingCb %d %d %d\n", addr, nregs, index);

  ret_helper = regReadWriteHolding(buf, nregs, index, read, reg);

  return ret_helper;
}
#endif
