// dawn/src/prog/buffer.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/buffer.hxx"

#include <cstring>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

uint32_t CProgBuffer::resolveRingIndex(const SBufferBind *bind, uint32_t sel, uint32_t depth)
{
  DAWNASSERT(bind != nullptr, "nullptr bind");
  DAWNASSERT(bind->count > 0, "count must be > 0");
  DAWNASSERT(depth > 0, "depth must be > 0");

  return (bind->head + depth - 1u - sel) % depth;
}

int CProgBuffer::srcNotifyCb(void *priv, io_ddata_t *data)
{
  CProgBuffer *self = static_cast<CProgBuffer *>(priv);
  if (self == nullptr || !self->bind->captureActive)
    {
      return OK;
    }

  return self->captureBind(data);
}

void CProgBuffer::outGetCb(CIOVirt *io, void *priv)
{
  (void)io;
  CProgBuffer *self = static_cast<CProgBuffer *>(priv);
  if (self == nullptr)
    {
      return;
    }

  int ret = self->updateSelected();
  if (ret != OK)
    {
      DAWNERR("buffer: out get update failed: %d\n", ret);
    }
}

void CProgBuffer::selSetCb(CIOVirt *io, void *priv)
{
  CProgBuffer *self = static_cast<CProgBuffer *>(priv);
  if (self == nullptr)
    {
      return;
    }

  SBufferBind *bind = self->bind;
  int ret = io->getVal(bind->selData->getDataPtr(), bind->selData->getDataSize());
  if (ret != OK)
    {
      DAWNERR("buffer: failed to read selector value: %d\n", ret);
      return;
    }

  bind->selectedOffset = bind->selData->get<uint32_t>(0);
  ret = self->updateSelected();
  if (ret != OK)
    {
      DAWNERR("buffer: selector apply failed: %d\n", ret);
    }
}

void CProgBuffer::statGetCb(CIOVirt *io, void *priv)
{
  (void)io;
  CProgBuffer *self = static_cast<CProgBuffer *>(priv);
  if (self == nullptr)
    {
      return;
    }

  int ret = self->updateStat();
  if (ret != OK)
    {
      DAWNERR("buffer: stat update failed: %d\n", ret);
    }
}

int CProgBuffer::allocBind(SObjectId::ObjectId src,
                           SObjectId::ObjectId out,
                           SObjectId::ObjectId sel,
                           SObjectId::ObjectId stat)
{
  bind = new (std::nothrow) SBufferBind();
  if (bind == nullptr)
    {
      return -ENOMEM;
    }

  bind->srcId = src;
  bind->outId = out;
  bind->selId = sel;
  bind->statId = stat;
  bind->src = nullptr;
  bind->out = nullptr;
  bind->sel = nullptr;
  bind->stat = nullptr;
  bind->ring = nullptr;
  bind->outData = nullptr;
  bind->selData = nullptr;
  bind->statData = nullptr;
  bind->head = 0;
  bind->count = 0;
  bind->overflow = 0;
  bind->selectedOffset = 0;
  bind->snapshotSeq = 0;
  bind->captureActive = false;

  setObjectMapItem(src, nullptr);
  setObjectMapItem(out, nullptr);
  setObjectMapItem(sel, nullptr);
  setObjectMapItem(stat, nullptr);

  return OK;
}

int CProgBuffer::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_BUFFER)
        {
          DAWNERR("buffer: unsupported cfg 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_BUFFER_CFG_IOBIND:
            {
              const size_t wpe = sizeof(SProgBufferIOBind) / 4;
              int ret;

              if (item->cfgid.s.size != wpe)
                {
                  DAWNERR("buffer: invalid IOBIND size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              if (bind != nullptr)
                {
                  DAWNERR("buffer: only one IOBIND per instance is supported\n");
                  return -EINVAL;
                }

              const SProgBufferIOBind *cfg =
                reinterpret_cast<const SProgBufferIOBind *>(item->data);

              ret = allocBind(cfg->src.v, cfg->out.v, cfg->sel.v, cfg->stat.v);
              if (ret != OK)
                {
                  return ret;
                }

              break;
            }

          case PROG_BUFFER_CFG_DEPTH:
            {
              const SObjectCfg::ObjectCfgData_t *data;

              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("buffer: invalid DEPTH size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              data = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
              depth = SObjectCfg::cfgToU32(data[0]);
              break;
            }

          case PROG_BUFFER_CFG_FLAGS:
            {
              const SObjectCfg::ObjectCfgData_t *data;

              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("buffer: invalid FLAGS size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              data = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
              flags = SObjectCfg::cfgToU32(data[0]);
              break;
            }

          case PROG_BUFFER_CFG_CHUNK_SIZE:
            {
              const SObjectCfg::ObjectCfgData_t *data;

              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("buffer: invalid CHUNK_SIZE size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              data = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
              chunkSize = SObjectCfg::cfgToU32(data[0]);
              break;
            }

          default:
            {
              DAWNERR("buffer: unsupported cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CProgBuffer::updateStat()
{
  uint32_t runtimeFlags = 0;
  bool full = (bind->count == depth);
  bool rangeError = (bind->count > 0 && bind->selectedOffset >= bind->count);

  if (getState() == STATE_RUNNING)
    {
      runtimeFlags |= RUNTIME_RUNNING;
    }

  if (bind->captureActive)
    {
      runtimeFlags |= RUNTIME_CAPTURE_ACTIVE;
    }

  if (full)
    {
      runtimeFlags |= RUNTIME_FULL;
    }

  if (rangeError)
    {
      runtimeFlags |= RUNTIME_ERROR_RANGE;
    }

  bind->statData->get<uint32_t>(STAT_COUNT) = bind->count;
  bind->statData->get<uint32_t>(STAT_DEPTH) = depth;
  bind->statData->get<uint32_t>(STAT_HEAD) = bind->head;
  bind->statData->get<uint32_t>(STAT_OVERFLOW) = bind->overflow;
  bind->statData->get<uint32_t>(STAT_SNAPSHOT_SEQ) = bind->snapshotSeq;
  bind->statData->get<uint32_t>(STAT_RUNTIME_FLAGS) = runtimeFlags;
  bind->statData->get<uint32_t>(STAT_SELECTED_OFFSET) = bind->selectedOffset;
  bind->statData->get<uint32_t>(STAT_RESERVED) = 0;

  return bind->stat->setVal(bind->statData->getDataPtr(), bind->statData->getDataSize());
}

int CProgBuffer::updateSelected()
{
  int ret;
  size_t sampleSize = bind->src->getDataSize();
  uint8_t *out = static_cast<uint8_t *>(bind->outData->getDataPtr());

  std::memset(bind->outData->getDataPtr(), 0, bind->outData->getDataSize());

  if (bind->count == 0)
    {
      bind->snapshotSeq++;
      ret = bind->out->setVal(bind->outData->getDataPtr(), bind->outData->getDataSize());
      if (ret != OK)
        {
          return ret;
        }

      return updateStat();
    }

  if (bind->selectedOffset >= bind->count)
    {
      updateStat();
      return -ERANGE;
    }

  for (uint32_t i = 0; i < chunkSize; i++)
    {
      uint32_t sel = bind->selectedOffset + i;
      if (sel >= bind->count)
        {
          break;
        }

      uint32_t idx = resolveRingIndex(bind, sel, depth);
      std::memcpy(out + (i * sampleSize), bind->ring->getDataPtr(idx), sampleSize);
    }

  bind->snapshotSeq++;

  ret = bind->out->setVal(bind->outData->getDataPtr(), bind->outData->getDataSize());
  if (ret != OK)
    {
      return ret;
    }

  return updateStat();
}

int CProgBuffer::captureBind(io_ddata_t *data)
{
  if (!bind->captureActive)
    {
      return OK;
    }

  if ((flags & FLAG_MODE_ONESHOT) && bind->count >= depth)
    {
      bind->captureActive = false;
      return updateStat();
    }

  std::memcpy(bind->ring->getDataPtr(bind->head), data->getDataPtr(), data->getDataSize());
  bind->ring->getTs(bind->head) = data->getTs(0);

  bind->head = (bind->head + 1u) % depth;
  if (bind->count < depth)
    {
      bind->count++;
    }
  else
    {
      bind->overflow++;
    }

  return updateSelected();
}

void CProgBuffer::clearBind()
{
  bind->head = 0;
  bind->count = 0;
  bind->overflow = 0;
  bind->selectedOffset = 0;
  bind->snapshotSeq = 0;

  std::memset(bind->outData->getDataPtr(), 0, bind->outData->getDataSize());
}

int CProgBuffer::validateBind()
{
  bind->src = getIO(bind->srcId);
  if (bind->src == nullptr)
    {
      DAWNERR("buffer: src 0x%" PRIx32 " not found\n", bind->srcId);
      return -EIO;
    }

  bind->out = reinterpret_cast<CIOVirt *>(getIO(bind->outId));
  if (bind->out == nullptr)
    {
      DAWNERR("buffer: out 0x%" PRIx32 " not found\n", bind->outId);
      return -EIO;
    }

  bind->sel = reinterpret_cast<CIOVirt *>(getIO(bind->selId));
  if (bind->sel == nullptr)
    {
      DAWNERR("buffer: sel 0x%" PRIx32 " not found\n", bind->selId);
      return -EIO;
    }

  bind->stat = reinterpret_cast<CIOVirt *>(getIO(bind->statId));
  if (bind->stat == nullptr)
    {
      DAWNERR("buffer: stat 0x%" PRIx32 " not found\n", bind->statId);
      return -EIO;
    }

  if (!bind->src->isNotify())
    {
      DAWNERR("buffer: src 0x%" PRIx32 " has no notify support\n", bind->src->getIdV());
      return -EINVAL;
    }

  if (bind->out->getDtype() != bind->src->getDtype())
    {
      DAWNERR("buffer: out IO incompatible with src 0x%" PRIx32 "\n", bind->src->getIdV());
      return -EINVAL;
    }

  if (bind->sel->getDtype() != SObjectId::DTYPE_UINT32)
    {
      DAWNERR("buffer: sel IO must use uint32 dtype\n");
      return -EINVAL;
    }

  if (bind->stat->getDtype() != SObjectId::DTYPE_UINT32)
    {
      DAWNERR("buffer: stat IO must use uint32 dtype\n");
      return -EINVAL;
    }

  return OK;
}

int CProgBuffer::allocateBind()
{
  int ret;
  const size_t srcDim = bind->src->getDataDim();
  size_t outDim;

  if (srcDim == 0)
    {
      DAWNERR("buffer: src dimension must be > 0\n");
      return -EINVAL;
    }

  outDim = srcDim * chunkSize;
  if (outDim == 0)
    {
      return -EINVAL;
    }

  if (bind->out->getCls() == CIOCommon::IO_CLASS_VIRT)
    {
      ret = bind->out->initialize(outDim, 1, true);
      if (ret != OK)
        {
          DAWNERR("buffer: out initialize failed: %d\n", ret);
          return ret;
        }
    }

  ret = prepareWritableTarget(bind->sel, 1, false);
  if (ret != OK)
    {
      DAWNERR("buffer: sel initialize failed: %d\n", ret);
      return ret;
    }

  ret = prepareWritableTarget(bind->stat, STAT_WORDS, false);
  if (ret != OK)
    {
      DAWNERR("buffer: stat initialize failed: %d\n", ret);
      return ret;
    }

  bind->ring = new (std::nothrow) io_ddata_t(
    bind->src->getDtypeSize(), srcDim, depth, bind->src->getDtype(), bind->src->isTimestamp());
  if (bind->ring == nullptr || !bind->ring->isAllocated())
    {
      delete bind->ring;
      bind->ring = nullptr;
      return -ENOMEM;
    }

  bind->outData = bind->out->ddata_alloc(1);
  bind->selData = bind->sel->ddata_alloc(1);
  bind->statData = bind->stat->ddata_alloc(1);

  if (bind->outData == nullptr || bind->selData == nullptr || bind->statData == nullptr)
    {
      return -ENOMEM;
    }

  return OK;
}

CProgBuffer::~CProgBuffer()
{
  deinit();
}

int CProgBuffer::configure()
{
  int ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("buffer: configure failed: %d\n", ret);
      return ret;
    }

  if (bind == nullptr)
    {
      DAWNERR("buffer: no binding configured\n");
      return -EINVAL;
    }

  if (depth == 0)
    {
      DAWNERR("buffer: depth must be > 0\n");
      return -EINVAL;
    }

  if (chunkSize == 0)
    {
      DAWNERR("buffer: chunk_size must be > 0\n");
      return -EINVAL;
    }

  if (chunkSize > depth)
    {
      DAWNERR("buffer: chunk_size must be <= depth\n");
      return -EINVAL;
    }

  uint32_t validFlags = FLAG_AUTO_START | FLAG_MODE_ONESHOT | FLAG_KEEP_DATA_ON_STOP;
  if ((flags & ~validFlags) != 0)
    {
      DAWNERR("buffer: unsupported FLAGS bits 0x%" PRIx32 "\n", flags & ~validFlags);
      return -EINVAL;
    }

  return OK;
}

int CProgBuffer::init()
{
  int ret;

  ret = validateBind();
  if (ret != OK)
    {
      return ret;
    }

  ret = allocateBind();
  if (ret != OK)
    {
      return ret;
    }

  ret = bind->src->setNotifier(srcNotifyCb, 0, this);
  if (ret != OK)
    {
      DAWNERR("buffer: set notifier failed for 0x%" PRIx32 ": %d\n", bind->src->getIdV(), ret);
      return ret;
    }

  return updateSelected();
}

int CProgBuffer::deinit()
{
  doStop();

  if (bind != nullptr)
    {
      delete bind->ring;
      delete bind->outData;
      delete bind->selData;
      delete bind->statData;
      delete bind;
      bind = nullptr;
    }

  return OK;
}

int CProgBuffer::doStart()
{
  bind->captureActive = (flags & FLAG_AUTO_START) != 0;

  if ((flags & FLAG_MODE_ONESHOT) && bind->count >= depth)
    {
      bind->captureActive = false;
    }

  bind->out->setCallbackGet(outGetCb, this);
  bind->sel->setCallbackSet(selSetCb, this);
  bind->stat->setCallbackGet(statGetCb, this);
  updateStat();

  return OK;
}

int CProgBuffer::doStop()
{
  bool keep = (flags & FLAG_KEEP_DATA_ON_STOP) != 0;

  if (bind == nullptr)
    {
      return OK;
    }

  bind->captureActive = false;

  if (bind->out != nullptr)
    {
      bind->out->setCallbackGet(nullptr, nullptr);
    }

  if (bind->sel != nullptr)
    {
      bind->sel->setCallbackSet(nullptr, nullptr);
    }

  if (bind->stat != nullptr)
    {
      bind->stat->setCallbackGet(nullptr, nullptr);
    }

  if (!keep && bind->ring != nullptr && bind->outData != nullptr)
    {
      clearBind();
    }

  if (bind->stat != nullptr && bind->statData != nullptr)
    {
      updateStat();
    }

  return OK;
}

bool CProgBuffer::hasThread() const
{
  return false;
}

void CProgBuffer::cmdReset()
{
  if (bind->ring != nullptr && bind->outData != nullptr)
    {
      clearBind();
      updateSelected();
    }
  if (bind->stat != nullptr && bind->statData != nullptr)
    {
      updateStat();
    }
}

void CProgBuffer::cmdStartCapture()
{
  if ((flags & FLAG_MODE_ONESHOT) && bind->count >= depth)
    {
      bind->captureActive = false;
    }
  else
    {
      bind->captureActive = true;
    }

  if (bind->stat != nullptr && bind->statData != nullptr)
    {
      updateStat();
    }
}

void CProgBuffer::cmdStopCapture()
{
  bind->captureActive = false;
  if (bind->stat != nullptr && bind->statData != nullptr)
    {
      updateStat();
    }
}

int CProgBuffer::trigger(uint8_t cmd)
{
  switch (cmd)
    {
      case CMD_RESET:
        cmdReset();
        return OK;

      case CMD_TRIGGER1:
        cmdStartCapture();
        return OK;

      case CMD_TRIGGER2:
        cmdStopCapture();
        return OK;

      default:
        return -ENOTSUP;
    }
}
