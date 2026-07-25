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

// Element widths with at least one enabled data type

#if defined(CONFIG_DAWN_DTYPE_BOOL) || defined(CONFIG_DAWN_DTYPE_INT8) || \
  defined(CONFIG_DAWN_DTYPE_UINT8)
#  define BITMERGE_HAVE_8BIT
#endif

#if defined(CONFIG_DAWN_DTYPE_INT16) || defined(CONFIG_DAWN_DTYPE_UINT16)
#  define BITMERGE_HAVE_16BIT
#endif

#if defined(CONFIG_DAWN_DTYPE_INT32) || defined(CONFIG_DAWN_DTYPE_UINT32)
#  define BITMERGE_HAVE_32BIT
#endif

// Integer or bool up to 32 bits: fields are merged as plain words

static bool isMergeDtype(uint8_t dtype)
{
  switch (dtype)
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
#endif
        return true;

      default:
        return false;
    }
}

// Read element zero of a buffer as a plain word; init() rejected other widths

static uint32_t readElem(io_ddata_t *data)
{
  switch (data->getSize())
    {
#ifdef BITMERGE_HAVE_8BIT
      case 1:
        {
          return *static_cast<uint8_t *>(data->getDataPtr());
        }
#endif

#ifdef BITMERGE_HAVE_16BIT
      case 2:
        {
          return *static_cast<uint16_t *>(data->getDataPtr());
        }
#endif

#ifdef BITMERGE_HAVE_32BIT
      case 4:
        {
          return *static_cast<uint32_t *>(data->getDataPtr());
        }
#endif

      default:
        {
          return 0;
        }
    }
}

// Store a merged word into element zero of a buffer

static void writeElem(io_ddata_t *data, uint32_t word)
{
  switch (data->getSize())
    {
#ifdef BITMERGE_HAVE_8BIT
      case 1:
        {
          *static_cast<uint8_t *>(data->getDataPtr()) = static_cast<uint8_t>(word);
          break;
        }
#endif

#ifdef BITMERGE_HAVE_16BIT
      case 2:
        {
          *static_cast<uint16_t *>(data->getDataPtr()) = static_cast<uint16_t>(word);
          break;
        }
#endif

#ifdef BITMERGE_HAVE_32BIT
      case 4:
        {
          *static_cast<uint32_t *>(data->getDataPtr()) = word;
          break;
        }
#endif

      default:
        {
          break;
        }
    }
}

// Merge a run of samples of one width: dst = ((src & mask) << shift) | orval

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

int CProgBitMerge::checkFieldLayout(uint32_t width) const
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

      if (inputs[i].shift >= width)
        {
          DAWNERR("bitmerge: input %zu shift %" PRIu32 " out of range\n", i, inputs[i].shift);
          return -EINVAL;
        }

      // Reject a mask shifted (even partly) out of the output word

      field = inputs[i].mask << inputs[i].shift;
      if ((field >> inputs[i].shift) != inputs[i].mask ||
          (width < FIELD_BITS && (field >> width) != 0))
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

      if (!isMergeDtype(io->getDtype()))
        {
          DAWNERR(
            "bitmerge: input 0x%" PRIx32 " unsupported dtype %u\n", inputs[i].ioId, io->getDtype());
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

          // Batches only arrive through notifications

          if (!io->isNotify())
            {
              DAWNERR("bitmerge: batched input 0x%" PRIx32 " is not notify-capable\n",
                      inputs[i].ioId);
              return -EINVAL;
            }

          streamIdx = i;
          batch = io->getNotifyBatch();
          outDim = io->getDataDim();
        }
      else if (io->getDataDim() > 1)
        {
          // Only element zero of a slow input enters the merge

          DAWNWARN("bitmerge: input 0x%" PRIx32 " dim %zu, using element 0\n",
                   inputs[i].ioId,
                   io->getDataDim());
        }
    }

  io = getIO(outputId);
  if (!io)
    {
      DAWNERR("bitmerge: output IO 0x%" PRIx32 " not found\n", outputId);
      return -EIO;
    }
  output = io;

  if (!isMergeDtype(output->getDtype()))
    {
      DAWNERR("bitmerge: output unsupported dtype %u\n", output->getDtype());
      return -EINVAL;
    }

  if (streamIdx != NO_STREAM)
    {
      CIOCommon *src = inputs[streamIdx].io;

      if (output->getDtypeSize() != src->getDtypeSize())
        {
          DAWNERR("bitmerge: output element size must match the batched input\n");
          return -EINVAL;
        }
    }

  // Fields must fit the real output width, not just a 32-bit word

  ret = checkFieldLayout(output->getDtypeSize() * CHAR_BIT);
  if (ret != OK)
    {
      return ret;
    }

  // Only a virt output can carry the batch, real IO writes batch 0 only

  if (output->getCls() != CIOCommon::IO_CLASS_VIRT)
    {
      batch = 1;
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

  if (inp->owner->streamIdx != NO_STREAM && inp == &inp->owner->inputs[inp->owner->streamIdx])
    {
      inp->owner->updateStream(data);
      return OK;
    }

  // Slow inputs are re-read per merge; the next batch picks up a change

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

              // Undo the notifiers already set

              while (i-- > 0)
                {
                  if (inputs[i].io->isNotify())
                    {
                      inputs[i].io->setNotifier(nullptr, 0, nullptr);
                    }
                }

              active = false;
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

int CProgBitMerge::mergeConstant(size_t skip, uint32_t &acc)
{
  size_t i;
  int ret;

  acc = 0;

  for (i = 0; i < inputs.size(); i++)
    {
      if (i == skip || inputs[i].currentData == nullptr)
        {
          continue;
        }

      ret = inputs[i].io->getData(*inputs[i].currentData, 1);
      if (ret != OK)
        {
          DAWNERR("bitmerge: getData failed for input 0x%" PRIx32 ": %d\n", inputs[i].ioId, ret);
          return ret;
        }

      acc |= (readElem(inputs[i].currentData) & inputs[i].mask) << inputs[i].shift;
    }

  return OK;
}

void CProgBitMerge::updateScalar()
{
  uint32_t word;
  int ret;

  if (output == nullptr || outputData == nullptr)
    {
      return;
    }

  // Notifiers may fire from different threads; one lock per update

  std::lock_guard<std::mutex> lock(mergeLock);

  if (mergeConstant(NO_STREAM, word) != OK)
    {
      return;
    }

  writeElem(outputData, word);

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
#ifdef BITMERGE_HAVE_8BIT
      case 1:
        {
          mergeSamplesT<uint8_t>(in, out, items, src->mask, src->shift, orval);
          break;
        }
#endif

#ifdef BITMERGE_HAVE_16BIT
      case 2:
        {
          mergeSamplesT<uint16_t>(in, out, items, src->mask, src->shift, orval);
          break;
        }
#endif

#ifdef BITMERGE_HAVE_32BIT
      case 4:
        {
          mergeSamplesT<uint32_t>(in, out, items, src->mask, src->shift, orval);
          break;
        }
#endif

      default:
        {
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
  size_t first;
  size_t flat;
  size_t b;
  int ret;

  if (data == nullptr || output == nullptr || outputData == nullptr)
    {
      return;
    }

  src = &inputs[streamIdx];

  // One lock per batch, not per sample

  std::lock_guard<std::mutex> lock(mergeLock);

  // Slow inputs are read once per batch and folded into one constant

  if (mergeConstant(streamIdx, orval) != OK)
    {
      return;
    }

  // An output holding fewer batches gets the latest samples

  nbatch = std::min(data->getBatch(), outputData->getBatch());
  first = data->getBatch() - nbatch;
  nitems = std::min(data->getItems(), outputData->getItems());

  // One flat pass when both sides are contiguous

  flat = first == 0 ? contiguousCount(data, nbatch, nitems) : 0;
  if (flat != 0 && contiguousCount(outputData, nbatch, nitems) == flat)
    {
      mergeBatch(data->getDataPtr(0), outputData->getDataPtr(0), flat, src, orval);
    }
  else
    {
      // Padded or timestamped batches are walked one by one

      for (b = 0; b < nbatch; b++)
        {
          mergeBatch(data->getDataPtr(first + b), outputData->getDataPtr(b), nitems, src, orval);
        }
    }

  if (outputData->hasTimestamp() && data->hasTimestamp())
    {
      for (b = 0; b < nbatch; b++)
        {
          outputData->getTs(b) = data->getTs(first + b);
        }
    }

  ret = output->setData(*outputData);
  if (ret != OK)
    {
      DAWNERR("bitmerge: setData on output failed %d\n", ret);
    }
}
