// dawn/src/prog/bitsplit.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/bitsplit.hxx"

#include <cstring>
#include <new>

#include "bitwise.hxx"
#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;
using namespace dawn::prog;

CProgBitSplit::CProgBitSplit(CDescObject &desc)
  : CProgCommon(desc)
  , active(false)
  , registered(false)
{
}

CProgBitSplit::~CProgBitSplit()
{
  deinit();
}

int CProgBitSplit::allocBind(SObjectId::ObjectId sourceId, SObjectId::ObjectId outputId)
{
  SBitSplitBind *bind;

  bind = new (std::nothrow) SBitSplitBind();
  if (!bind)
    {
      return -ENOMEM;
    }

  bind->owner = this;
  bind->sourceId = sourceId;
  bind->outputId = outputId;
  bind->source = nullptr;
  bind->output = nullptr;
  bind->sourceData = nullptr;
  bind->outputData = nullptr;
  bind->bit = 0;

  binds.push_back(bind);
  setObjectMapItem(sourceId, nullptr);
  setObjectMapItem(outputId, nullptr);
  return OK;
}

int CProgBitSplit::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  const SObjectCfg::ObjectCfgData_t *raw;
  size_t offset;
  size_t ii;
  size_t nitems;

  offset = 0;
  for (ii = 0; ii < desc.getSize(); ii++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_BITSPLIT)
        {
          DAWNERR("bitsplit: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_BITSPLIT_CFG_IOBIND:
            {
              nitems = static_cast<size_t>(item->cfgid.s.size);
              if (nitems == 0 || nitems % 2 != 0)
                {
                  DAWNERR("bitsplit: invalid IOBIND size %zu\n", nitems);
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              for (size_t b = 0; b < nitems / 2; b++)
                {
                  int ret = allocBind(ids[b * 2].v, ids[b * 2 + 1].v);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }
              break;
            }

          case PROG_BITSPLIT_CFG_BITS:
            {
              nitems = static_cast<size_t>(item->cfgid.s.size);
              if (nitems == 0)
                {
                  DAWNERR("bitsplit: BITS config must have at least one entry\n");
                  return -EINVAL;
                }

              raw = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
              bitPositions.reserve(nitems);
              for (size_t i = 0; i < nitems; i++)
                {
                  bitPositions.push_back(SObjectCfg::cfgToU32(raw[i]));
                }
              break;
            }

          default:
            {
              DAWNERR("bitsplit: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (binds.empty())
    {
      DAWNERR("bitsplit: at least one IOBIND entry required\n");
      return -EINVAL;
    }

  if (binds.size() != bitPositions.size())
    {
      DAWNERR(
        "bitsplit: IOBIND count (%zu) != BITS count (%zu)\n", binds.size(), bitPositions.size());
      return -EINVAL;
    }

  for (ii = 0; ii < binds.size(); ii++)
    {
      binds[ii]->bit = bitPositions[ii];
    }

  return OK;
}

int CProgBitSplit::configure()
{
  return configureDesc(getDesc());
}

int CProgBitSplit::init()
{
  int ret;

  for (auto bind : binds)
    {
      bind->source = getIO(bind->sourceId);
      if (!bind->source)
        {
          DAWNERR("bitsplit: source IO 0x%" PRIx32 " not found\n", bind->sourceId);
          return -EIO;
        }

      if (!bind->source->isRead() || !isBitwiseDtype(bind->source->getDtype()))
        {
          DAWNERR("bitsplit: source IO 0x%" PRIx32 " is not readable bitwise IO\n", bind->sourceId);
          return -EINVAL;
        }

      bind->output = getIO(bind->outputId);
      if (!bind->output)
        {
          DAWNERR("bitsplit: output IO 0x%" PRIx32 " not found\n", bind->outputId);
          return -EIO;
        }

      if (!isBitwiseDtype(bind->output->getDtype()))
        {
          DAWNERR("bitsplit: output IO 0x%" PRIx32 " unsupported dtype %u\n",
                  bind->outputId,
                  bind->output->getDtype());
          return -EINVAL;
        }

      ret = prepareWritableTarget(
        bind->output, bind->output->getDataDim() == 0 ? 1 : bind->output->getDataDim(), true);
      if (ret != OK)
        {
          DAWNERR("bitsplit: output target prepare failed %d\n", ret);
          return ret;
        }

      bind->sourceData = bind->source->ddata_alloc(1);
      bind->outputData = bind->output->ddata_alloc(1);
      if (!bind->sourceData || !bind->outputData)
        {
          DAWNERR("bitsplit: data allocation failed\n");
          return -ENOMEM;
        }
    }

  return OK;
}

int CProgBitSplit::deinit()
{
  doStop();

  for (auto bind : binds)
    {
      delete bind->sourceData;
      delete bind->outputData;
      delete bind;
    }

  binds.clear();
  bitPositions.clear();
  return OK;
}

int CProgBitSplit::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SBitSplitBind *bind = static_cast<SBitSplitBind *>(priv);

  if (!bind || !bind->owner || !bind->owner->active)
    {
      return OK;
    }

  if (data && data->getItems() >= 1)
    {
      bind->owner->refreshSource(bind->sourceId);
    }

  return OK;
}

int CProgBitSplit::doStart()
{
  int ret;

  if (!registered)
    {
      for (auto bind : binds)
        {
          if (!bind->source->isNotify())
            {
              continue;
            }

          ret = bind->source->setNotifier(ioNotifierCb, 0, bind);
          if (ret != OK)
            {
              DAWNERR(
                "bitsplit: setNotifier failed for source 0x%" PRIx32 ": %d\n", bind->sourceId, ret);
              return ret;
            }
        }

      registered = true;
    }

  active = true;

  for (auto bind : binds)
    {
      updateBind(bind, nullptr);
    }

  return OK;
}

int CProgBitSplit::doStop()
{
  if (registered)
    {
      for (auto bind : binds)
        {
          if (bind->source && bind->source->isNotify())
            {
              bind->source->setNotifier(nullptr, 0, nullptr);
            }
        }

      registered = false;
    }

  active = false;
  return OK;
}

bool CProgBitSplit::hasThread() const
{
  return false;
}

void CProgBitSplit::refreshSource(SObjectId::ObjectId sourceId)
{
  for (auto bind : binds)
    {
      if (bind->sourceId == sourceId)
        {
          updateBind(bind, nullptr);
        }
    }
}

void CProgBitSplit::updateBind(SBitSplitBind *bind, io_ddata_t *data)
{
  size_t srcBits;
  size_t dstBits;
  size_t bit;
  int ret;

  if (!bind || !bind->source || !bind->output || !bind->sourceData || !bind->outputData)
    {
      return;
    }

  if (data && data->getItems() >= 1)
    {
      std::memcpy(
        bind->sourceData->getDataPtr(), data->getDataPtr(), bind->sourceData->getDataSize());
    }
  else
    {
      ret = bind->source->getData(*bind->sourceData, 1);
      if (ret != OK)
        {
          DAWNERR("bitsplit: getData failed for source 0x%" PRIx32 ": %d\n", bind->sourceId, ret);
          return;
        }
    }

  srcBits = getLogicalBits(bind->source);
  dstBits = getLogicalBits(bind->output);

  if (static_cast<size_t>(bind->bit) >= srcBits)
    {
      DAWNERR("bitsplit: start bit %" PRIu32 " out of range for 0x%" PRIx32 " (%zu bits)\n",
              bind->bit,
              bind->outputId,
              srcBits);
      return;
    }

  std::memset(bind->outputData->getDataPtr(), 0, bind->outputData->getDataSize());

  for (bit = 0; bit < dstBits && static_cast<size_t>(bind->bit) + bit < srcBits; bit++)
    {
      if (readLogicalBit(
            bind->source, bind->sourceData->getDataPtr(), static_cast<size_t>(bind->bit) + bit))
        {
          writeLogicalBit(bind->output, bind->outputData->getDataPtr(), bit, true);
        }
    }

  ret = bind->output->setData(*bind->outputData);
  if (ret != OK)
    {
      DAWNERR("bitsplit: setData failed %d\n", ret);
    }
}
