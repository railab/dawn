// dawn/src/prog/bitmerge.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/bitmerge.hxx"

#include <algorithm>
#include <climits>
#include <cstring>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

static const size_t INPUT_WORDS = 3; ///< Words per input (ioId + shift + mask).
static const uint32_t FIELD_BITS = sizeof(uint32_t) * CHAR_BIT;

/* Read element zero of a buffer as a plain word. */

static uint32_t readElem(io_ddata_t *data)
{
  switch (data->getSize())
    {
      case 1:
        {
          return *static_cast<uint8_t *>(data->getDataPtr());
        }

      case 2:
        {
          return *static_cast<uint16_t *>(data->getDataPtr());
        }

      default:
        {
          return *static_cast<uint32_t *>(data->getDataPtr());
        }
    }
}

template<typename T>
static void mergeSamplesT(const void *in,
                          void *out,
                          size_t items,
                          uint32_t mask,
                          uint32_t shift,
                          uint32_t orval)
{
  const T *src = static_cast<const T *>(in);
  T *dst = static_cast<T *>(out);
  size_t i;

  for (i = 0; i < items; i++)
    {
      dst[i] = static_cast<T>(((static_cast<uint32_t>(src[i]) & mask) << shift) | orval);
    }
}

CProgBitMerge::CProgBitMerge(CDescObject &desc)
  : CProgCommon(desc)
  , output(nullptr)
  , outputId(0)
  , outputData(nullptr)
  , lastOutputData(nullptr)
  , streamIdx(NO_STREAM)
  , batch(1)
  , active(false)
  , registered(false)
{
}

CProgBitMerge::~CProgBitMerge()
{
  deinit();
}

int CProgBitMerge::allocInput(SObjectId::ObjectId ioId, uint32_t shift, uint32_t mask)
{
  SMergeInput inp;

  inp.owner = this;
  inp.io = nullptr;
  inp.ioId = ioId;
  inp.shift = shift;
  inp.mask = mask;
  inp.currentData = nullptr;

  inputs.push_back(inp);
  setObjectMapItem(ioId, nullptr);
  return OK;
}

int CProgBitMerge::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  const uint32_t *vals;
  size_t offset;
  size_t ii;
  size_t n;

  offset = 0;
  for (ii = 0; ii < desc.getSize(); ii++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_BITMERGE)
        {
          DAWNERR("bitmerge: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_BITMERGE_CFG_INPUTS:
            {
              n = static_cast<size_t>(item->cfgid.s.size);
              if (n == 0 || n % INPUT_WORDS != 0)
                {
                  DAWNERR("bitmerge: invalid INPUTS size %zu\n", n);
                  return -EINVAL;
                }

              vals = reinterpret_cast<const uint32_t *>(item->data);
              for (size_t j = 0; j < n; j += INPUT_WORDS)
                {
                  ids = reinterpret_cast<const SObjectId::UObjectId *>(&vals[j]);
                  int ret = allocInput(ids[0].v, vals[j + 1], vals[j + 2]);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }
              break;
            }

          case PROG_BITMERGE_CFG_OUTPUT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("bitmerge: OUTPUT must have 1 entry\n");
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              outputId = ids[0].v;
              setObjectMapItem(outputId, nullptr);
              break;
            }

          default:
            {
              DAWNERR("bitmerge: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (inputs.empty())
    {
      DAWNERR("bitmerge: at least one input required\n");
      return -EINVAL;
    }

  if (outputId == 0)
    {
      DAWNERR("bitmerge: output IO not configured\n");
      return -EINVAL;
    }

  return OK;
}

int CProgBitMerge::configure()
{
  return configureDesc(getDesc());
}

int CProgBitMerge::checkFieldLayout() const
{
  uint32_t used = 0;
  size_t i;

  for (i = 0; i < inputs.size(); i++)
    {
      uint32_t field;

      if (inputs[i].mask == 0)
        {
          DAWNERR("bitmerge: input %zu has an empty mask\n", i);
          return -EINVAL;
        }

      if (inputs[i].shift >= FIELD_BITS)
        {
          DAWNERR("bitmerge: input %zu shift %" PRIu32 " out of range\n", i, inputs[i].shift);
          return -EINVAL;
        }

      // Reject a mask that would be shifted out of the word entirely or in
      // part - a silently truncated field is the bug this program exists to
      // avoid.

      field = inputs[i].mask << inputs[i].shift;
      if ((field >> inputs[i].shift) != inputs[i].mask)
        {
          DAWNERR("bitmerge: input %zu field does not fit the word\n", i);
          return -EINVAL;
        }

      if ((used & field) != 0)
        {
          DAWNERR("bitmerge: input %zu field overlaps an earlier one\n", i);
          return -EINVAL;
        }

      used |= field;
    }

  return OK;
}

int CProgBitMerge::init()
{
  CIOCommon *io;
  size_t outDim = 1;
  size_t i;
  int ret;

  ret = checkFieldLayout();
  if (ret != OK)
    {
      return ret;
    }

  for (i = 0; i < inputs.size(); i++)
    {
      io = getIO(inputs[i].ioId);
      if (!io)
        {
          DAWNERR("bitmerge: input IO 0x%" PRIx32 " not found\n", inputs[i].ioId);
          return -EIO;
        }

      if (!io->isRead())
        {
          DAWNERR("bitmerge: input 0x%" PRIx32 " is not readable\n", inputs[i].ioId);
          return -EINVAL;
        }

      inputs[i].io = io;

      // Batching is a property of the input. The batched one drives the
      // output rate and shape; the rest are slow operands.

      if (io->isBatch())
        {
          if (streamIdx != NO_STREAM)
            {
              DAWNERR("bitmerge: only one batched input is supported\n");
              return -EINVAL;
            }

          streamIdx = i;
          batch = io->getNotifyBatch();
          outDim = io->getDataDim();
        }
    }

  io = getIO(outputId);
  if (!io)
    {
      DAWNERR("bitmerge: output IO 0x%" PRIx32 " not found\n", outputId);
      return -EIO;
    }
  output = io;

  if (streamIdx != NO_STREAM)
    {
      CIOCommon *src = inputs[streamIdx].io;

      if (output->getDtypeSize() != src->getDtypeSize())
        {
          DAWNERR("bitmerge: output element size must match the batched input\n");
          return -EINVAL;
        }
    }

  if (output->getDtypeSize() > sizeof(uint32_t))
    {
      DAWNERR("bitmerge: output element size %zu not supported\n", output->getDtypeSize());
      return -ENOTSUP;
    }

  ret = prepareWritableTarget(output, outDim, true, batch);
  if (ret != OK)
    {
      DAWNERR("bitmerge: output target prepare failed %d\n", ret);
      return ret;
    }

  for (i = 0; i < inputs.size(); i++)
    {
      inputs[i].currentData = inputs[i].io->ddata_alloc(1);
      if (inputs[i].currentData == nullptr)
        {
          DAWNERR("bitmerge: currentData allocation failed for input %zu\n", i);
          return -ENOMEM;
        }
    }

  outputData = output->ddata_alloc(batch);
  if (outputData == nullptr)
    {
      DAWNERR("bitmerge: outputData allocation failed\n");
      return -ENOMEM;
    }

  // Publish-on-change only applies to the scalar case; a stream must emit
  // every batch even when consecutive batches are identical.

  if (streamIdx == NO_STREAM)
    {
      lastOutputData = output->ddata_alloc(1);
      if (lastOutputData == nullptr)
        {
          DAWNERR("bitmerge: lastOutputData allocation failed\n");
          return -ENOMEM;
        }

      std::memset(lastOutputData->getDataPtr(), 0xff, lastOutputData->getDataSize());
    }

  return OK;
}

int CProgBitMerge::deinit()
{
  doStop();

  for (size_t i = 0; i < inputs.size(); i++)
    {
      delete inputs[i].currentData;
      inputs[i].currentData = nullptr;
    }

  inputs.clear();

  delete outputData;
  outputData = nullptr;
  delete lastOutputData;
  lastOutputData = nullptr;
  output = nullptr;
  outputId = 0;
  streamIdx = NO_STREAM;
  batch = 1;
  return OK;
}

int CProgBitMerge::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SMergeInput *inp = static_cast<SMergeInput *>(priv);

  if (inp == nullptr || inp->owner == nullptr || !inp->owner->active)
    {
      return OK;
    }

  if (inp->owner->streamIdx != NO_STREAM && inp->owner->inputs[inp->owner->streamIdx].io == inp->io)
    {
      inp->owner->updateStream(data);
      return OK;
    }

  if (data && data->getItems() >= 1 && inp->currentData)
    {
      std::memcpy(
        inp->currentData->getDataPtr(), data->getDataPtr(), inp->currentData->getDataSize());
    }

  // A slow input changing does not republish the stream; the next batch
  // picks the new value up.

  if (inp->owner->streamIdx == NO_STREAM)
    {
      inp->owner->updateScalar();
    }

  return OK;
}

int CProgBitMerge::doStart()
{
  size_t i;
  int ret;

  active = true;

  if (!registered)
    {
      for (i = 0; i < inputs.size(); i++)
        {
          if (!inputs[i].io->isNotify())
            {
              continue;
            }

          ret = inputs[i].io->setNotifier(ioNotifierCb, 0, &inputs[i]);
          if (ret != OK)
            {
              DAWNERR("bitmerge: setNotifier failed on input %zu: %d\n", i, ret);
              return ret;
            }
        }

      registered = true;
    }

  if (streamIdx == NO_STREAM)
    {
      updateScalar();
    }

  return OK;
}

int CProgBitMerge::doStop()
{
  if (registered)
    {
      for (size_t i = 0; i < inputs.size(); i++)
        {
          if (inputs[i].io && inputs[i].io->isNotify())
            {
              inputs[i].io->setNotifier(nullptr, 0, nullptr);
            }
        }

      registered = false;
    }

  active = false;
  return OK;
}

bool CProgBitMerge::hasThread() const
{
  return false;
}

uint32_t CProgBitMerge::mergeConstant(size_t skip)
{
  uint32_t acc = 0;
  size_t i;

  for (i = 0; i < inputs.size(); i++)
    {
      if (i == skip || inputs[i].currentData == nullptr)
        {
          continue;
        }

      if (inputs[i].io->getData(*inputs[i].currentData, 1) != OK)
        {
          continue;
        }

      acc |= (readElem(inputs[i].currentData) & inputs[i].mask) << inputs[i].shift;
    }

  return acc;
}

void CProgBitMerge::updateScalar()
{
  uint32_t word;
  int ret;

  if (output == nullptr || outputData == nullptr)
    {
      return;
    }

  word = mergeConstant(NO_STREAM);

  switch (outputData->getSize())
    {
      case 1:
        {
          *static_cast<uint8_t *>(outputData->getDataPtr()) = static_cast<uint8_t>(word);
          break;
        }

      case 2:
        {
          *static_cast<uint16_t *>(outputData->getDataPtr()) = static_cast<uint16_t>(word);
          break;
        }

      default:
        {
          *static_cast<uint32_t *>(outputData->getDataPtr()) = word;
          break;
        }
    }

  // Publish only on change - periodic producers would otherwise turn the
  // output into a fixed-rate stream.

  if (lastOutputData != nullptr)
    {
      if (std::memcmp(
            outputData->getDataPtr(), lastOutputData->getDataPtr(), outputData->getDataSize()) == 0)
        {
          return;
        }

      std::memcpy(
        lastOutputData->getDataPtr(), outputData->getDataPtr(), outputData->getDataSize());
    }

  ret = output->setData(*outputData);
  if (ret != OK)
    {
      DAWNERR("bitmerge: setData on output failed %d\n", ret);
    }
}

void CProgBitMerge::mergeBatch(const void *in,
                               void *out,
                               size_t items,
                               const SMergeInput *src,
                               uint32_t orval)
{
  switch (outputData->getSize())
    {
      case 1:
        {
          mergeSamplesT<uint8_t>(in, out, items, src->mask, src->shift, orval);
          break;
        }

      case 2:
        {
          mergeSamplesT<uint16_t>(in, out, items, src->mask, src->shift, orval);
          break;
        }

      default:
        {
          mergeSamplesT<uint32_t>(in, out, items, src->mask, src->shift, orval);
          break;
        }
    }
}

void CProgBitMerge::updateStream(io_ddata_t *data)
{
  const SMergeInput *src;
  uint32_t orval;
  size_t nbatch;
  size_t nitems;
  size_t flat;
  size_t b;
  int ret;

  if (data == nullptr || output == nullptr || outputData == nullptr)
    {
      return;
    }

  src = &inputs[streamIdx];

  // One read of every slow input per batch, folded into a single constant so
  // the sample loop stays a load/and/shift/or.

  orval = mergeConstant(streamIdx);

  nbatch = std::min(data->getBatch(), outputData->getBatch());
  nitems = std::min(data->getItems(), outputData->getItems());

  // One pass over the whole buffer when both sides are contiguous - walking
  // per batch costs a dispatch per sample on a dim-1 stream.

  flat = flatItems(data, nbatch, nitems);
  if (flat != 0 && flatItems(outputData, nbatch, nitems) == flat)
    {
      mergeBatch(data->getDataPtr(0), outputData->getDataPtr(0), flat, src, orval);
    }
  else
    {
      // Padded or timestamped batches: only getDataPtr() knows where the
      // elements of each one start.

      for (b = 0; b < nbatch; b++)
        {
          mergeBatch(data->getDataPtr(b), outputData->getDataPtr(b), nitems, src, orval);
        }
    }

  ret = output->setData(*outputData);
  if (ret != OK)
    {
      DAWNERR("bitmerge: setData on output failed %d\n", ret);
    }
}
