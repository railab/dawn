// dawn/src/prog/sequencer.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/sequencer.hxx"

#include <cstring>
#include <new>
#include <unistd.h>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

int CProgSequencer::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;
  bool hasTargets = false;
  bool hasStates = false;
  size_t statesCount = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_SEQUENCER)
        {
          DAWNERR("sequencer: unsupported cfg 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_SEQUENCER_CFG_TARGET:
            {
              const SObjectId::UObjectId *ids;
              size_t ntargets;
              size_t j;
              int ret;

              if (item->cfgid.s.size == 0)
                {
                  DAWNERR("sequencer: no targets configured\n");
                  return -EINVAL;
                }

              hasTargets = true;
              ntargets = item->cfgid.s.size;
              targetIds.reserve(ntargets);
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);

              for (j = 0; j < ntargets; j++)
                {
                  ret = allocObject(ids[j].v);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }

              break;
            }

          case PROG_SEQUENCER_CFG_STATE:
            {
              if (item->cfgid.s.size == 0 || item->cfgid.s.size % 2 != 0)
                {
                  DAWNERR("sequencer: invalid state table size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              hasStates = true;
              statesCount = item->cfgid.s.size / 2;
              break;
            }

          case PROG_SEQUENCER_CFG_START:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("sequencer: invalid START size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              break;
            }

          default:
            {
              DAWNERR("sequencer: unsupported cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  if (!hasTargets || targetIds.empty())
    {
      DAWNERR("sequencer: target list is empty\n");
      return -EINVAL;
    }

  if (!hasStates || statesCount == 0)
    {
      DAWNERR("sequencer: state table is empty\n");
      return -EINVAL;
    }

  states.resize(statesCount);

  return reloadRuntimeConfig(true);
}

int CProgSequencer::allocObject(SObjectId::ObjectId targetId)
{
  DAWNINFO("allocate sequencer target 0x%" PRIx32 "\n", targetId);

  targetIds.push_back(targetId);
  setObjectMapItem(targetId, nullptr);

  return OK;
}

int CProgSequencer::reloadRuntimeConfig(bool resetCurrent,
                                        SObjectCfg::ObjectCfgId overrideCfg,
                                        const uint32_t *overrideData,
                                        size_t overrideLen)
{
  CDescObject &desc = getDesc();
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  const uint32_t *data = nullptr;
  size_t dataWords = 0;
  size_t offset = 0;
  size_t statesCount = 0;
  size_t newStartIndex = startIndex;
  size_t parsedStates = 0;
  size_t i;
  bool hasState = false;
  bool hasStartItem = false;

  for (size_t itemIdx = 0; itemIdx < desc.getSize(); itemIdx++)
    {
      item = desc.objectCfgItemNext(offset);
      data = reinterpret_cast<const uint32_t *>(item->data);
      dataWords = item->cfgid.s.size;

      if (overrideCfg != 0 && item->cfgid.v == overrideCfg)
        {
          if (!overrideData)
            {
              DAWNERR("sequencer: null override data\n");
              return -EINVAL;
            }

          data = overrideData;
          dataWords = overrideLen;
        }

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_SEQUENCER)
        {
          DAWNERR("sequencer: invalid runtime cfg class\n");
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_SEQUENCER_CFG_STATE:
            {
              hasState = true;
              statesCount = dataWords / 2;
              if (dataWords == 0 || dataWords % 2 != 0 || states.empty() ||
                  statesCount != states.size())
                {
                  DAWNERR("sequencer: invalid state table size %d\n", (int)dataWords);
                  return -EINVAL;
                }

              parsedStates = statesCount;
              break;
            }

          case PROG_SEQUENCER_CFG_START:
            {
              if (dataWords != 1)
                {
                  DAWNERR("sequencer: invalid START size %d\n", (int)dataWords);
                  return -EINVAL;
                }

              hasStartItem = true;
              if (overrideCfg == 0 || item->cfgid.v == overrideCfg)
                {
                  newStartIndex = static_cast<size_t>(*data);
                }
              break;
            }

          default:
            break;
        }
    }

  if (!hasState || parsedStates == 0)
    {
      DAWNERR("sequencer: state table is empty\n");
      return -EINVAL;
    }

  if (hasStartItem && newStartIndex >= parsedStates)
    {
      DAWNERR("sequencer: start index %zu out of range (%zu)\n", newStartIndex, parsedStates);
      return -EINVAL;
    }

  offset = 0;
  stateLock.lock();

  for (size_t itemIdx = 0; itemIdx < desc.getSize(); itemIdx++)
    {
      item = desc.objectCfgItemNext(offset);
      data = reinterpret_cast<const uint32_t *>(item->data);
      dataWords = item->cfgid.s.size;

      if (overrideCfg != 0 && item->cfgid.v == overrideCfg)
        {
          data = overrideData;
          dataWords = overrideLen;
        }

      if (item->cfgid.s.id == PROG_SEQUENCER_CFG_STATE)
        {
          parsedStates = 0;
          for (i = 0; i < dataWords; i += 2)
            {
              states[parsedStates].value = data[i];
              states[parsedStates].dwellUs = data[i + 1];
              if (states[parsedStates].dwellUs == 0)
                {
                  DAWNERR("sequencer: state[%zu] dwell_us must be > 0\n", parsedStates);
                  stateLock.unlock();
                  return -EINVAL;
                }

              parsedStates++;
            }
        }
    }

  startIndex = newStartIndex;

  if (resetCurrent)
    {
      currentIndex = startIndex;
    }

  stateLock.unlock();
  return OK;
}

int CProgSequencer::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  int ret;
  uint8_t id;
  bool resetCurrent;

  if (SObjectCfg::objectCfgGetType(objcfg) != SObjectId::OBJTYPE_PROG)
    {
      return OK;
    }

  if (SObjectCfg::objectCfgGetCls(objcfg) != CProgCommon::PROG_CLASS_SEQUENCER)
    {
      return OK;
    }

  id = SObjectCfg::objectCfgGetId(objcfg);
  if (id != PROG_SEQUENCER_CFG_STATE && id != PROG_SEQUENCER_CFG_START)
    {
      return OK;
    }

  resetCurrent = id == PROG_SEQUENCER_CFG_START || getState() != CObject::STATE_RUNNING;

  ret = reloadRuntimeConfig(resetCurrent, objcfg, data, len);
  if (ret != OK)
    {
      return ret;
    }

  if (getState() == CObject::STATE_STOPPED)
    {
      ret = applyState(startIndex);
      if (ret != OK)
        {
          return ret;
        }
    }
  else if (getState() == CObject::STATE_RUNNING)
    {
      size_t index;

      stateLock.lock();
      index = currentIndex;
      stateLock.unlock();

      ret = applyState(index);
      if (ret != OK)
        {
          return ret;
        }
    }

  return OK;
}

int CProgSequencer::applyState(size_t index)
{
  uint32_t value;
  int ret;
  size_t i;

  if (index >= states.size() || !iodata || targetSize == 0 || targetSize > 4)
    {
      return -EINVAL;
    }

  value = states[index].value;
  std::memset(iodata->getDataPtr(), 0, iodata->getDataSize());
  std::memcpy(iodata->getDataPtr(), &value, targetSize);

  ret = OK;
  for (i = 0; i < targets.size(); i++)
    {
      int r;
      r = targets[i]->setData(*iodata);
      if (r != OK)
        {
          DAWNERR("sequencer: setData failed for target[%zu] (%d)\n", i, r);
          ret = r;
        }
    }

  return ret;
}

void CProgSequencer::thread()
{
  size_t idx;
  uint32_t dwell;

  DAWNINFO("start sequencer thread\n");

  do
    {
      stateLock.lock();
      idx = currentIndex;
      dwell = states[idx].dwellUs;
      stateLock.unlock();

      applyState(idx);
      usleep(dwell);

      stateLock.lock();
      currentIndex = (idx + 1) % states.size();
      stateLock.unlock();
    }
  while (!threadCtl.shouldQuit());
}

CProgSequencer::~CProgSequencer()
{
  deinit();
}

int CProgSequencer::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("sequencer configure failed: %d\n", ret);
      return ret;
    }

  return OK;
}

int CProgSequencer::init()
{
  CIOCommon *io;
  size_t i;
  int ret;

  targets.reserve(targetIds.size());

  for (i = 0; i < targetIds.size(); i++)
    {
      io = getIO(targetIds[i]);
      if (!io)
        {
          DAWNERR("sequencer: target 0x%" PRIx32 " not found\n", targetIds[i]);
          return -EIO;
        }

      ret = prepareWritableTarget(io, 1, true);
      if (ret != OK)
        {
          DAWNERR("sequencer: target prepare failed %d\n", ret);
          return ret;
        }

      if (io->getDataDim() != 1)
        {
          DAWNERR("sequencer: target 0x%" PRIx32 " must be scalar output\n", targetIds[i]);
          return -EINVAL;
        }

      if (io->getDataSize() == 0 || io->getDataSize() > sizeof(uint32_t))
        {
          DAWNERR("sequencer: target 0x%" PRIx32 " unsupported data size %zu (expected 1..4)\n",
                  targetIds[i],
                  io->getDataSize());
          return -EINVAL;
        }

      if (targets.empty())
        {
          targetSize = io->getDataSize();
          targetDtype = io->getDtype();
        }
      else if (targetSize != io->getDataSize() || targetDtype != io->getDtype())
        {
          DAWNERR("sequencer: target 0x%" PRIx32 " type mismatch with first target\n",
                  targetIds[i]);
          return -EINVAL;
        }

      targets.push_back(io);
    }

  // Target ID list is no longer needed after targets are resolved.
  std::vector<SObjectId::ObjectId>().swap(targetIds);

  iodata = new (std::nothrow) io_ddata_t(targetSize, 1, 1, targetDtype);
  if (iodata == nullptr || !iodata->isAllocated())
    {
      delete iodata;
      iodata = nullptr;
      DAWNERR("sequencer: iodata allocation failed\n");
      return -ENOMEM;
    }

  return OK;
}

int CProgSequencer::deinit()
{
  doStop();
  delete iodata;
  iodata = nullptr;
  std::vector<CIOCommon *>().swap(targets);
  std::vector<SObjectId::ObjectId>().swap(targetIds);
  std::vector<SState>().swap(states);
  targetSize = 0;
  targetDtype = SObjectId::DTYPE_ANY;
  return OK;
}

int CProgSequencer::doStart()
{
  int ret;

  ret = reloadRuntimeConfig(true);
  if (ret != OK)
    {
      return ret;
    }

  // The worker thread applies the start state on its first iteration.
  // Not writing here avoids a stale value when another PROG (e.g.
  // Switch) immediately stops this sequencer before the thread runs.

  threadCtl.setThreadFunc([this]() { thread(); });
  return threadCtl.threadStart();
}

int CProgSequencer::doStop()
{
  return threadCtl.threadStop();
}

bool CProgSequencer::hasThread() const
{
  return threadCtl.isRunning();
}

int CProgSequencer::trigger(uint8_t cmd)
{
  int ret;

  if (cmd != CMD_RESET)
    {
      return -ENOTSUP;
    }

  ret = reloadRuntimeConfig(true);
  if (ret != OK)
    {
      return ret;
    }

  ret = applyState(startIndex);
  return ret;
}
