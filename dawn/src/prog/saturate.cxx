// dawn/src/prog/saturate.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/saturate.hxx"

#include <algorithm>
#include <cstring>
#include <limits>
#include <type_traits>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

/* Value range of a payload data type, plus whether descriptor words holding
 * a bound for it are two's complement.
 */

struct SDtypeRange
{
  int64_t lo;
  int64_t hi;
  bool sign;
};

/* Clamp in the input type, then store in the output type. Bounds are decoded
 * once as int64_t and narrowed here; comparing per sample in int64_t costs
 * ~20% of the stream rate on a 32-bit core. resolveBounds() has proven the
 * bounds fit both types, so the store cannot truncate.
 */

template<typename TIN, typename TOUT>
static void saturateT(const void *in, void *out, size_t items, int64_t lo64, int64_t hi64)
{
  typedef typename std::
    conditional<sizeof(TIN) >= 4 && !std::is_signed<TIN>::value, int64_t, int32_t>::type W;
  const TIN *src = static_cast<const TIN *>(in);
  TOUT *dst = static_cast<TOUT *>(out);
  const W lo = static_cast<W>(lo64);
  const W hi = static_cast<W>(hi64);
  size_t i;

  for (i = 0; i < items; i++)
    {
      W v = static_cast<W>(src[i]);

      if (v < lo)
        {
          v = lo;
        }
      else if (v > hi)
        {
          v = hi;
        }

      dst[i] = static_cast<TOUT>(v);
    }
}

template<typename T>
static void dtypeRangeOf(SDtypeRange &r, bool sign)
{
  r.lo = static_cast<int64_t>(std::numeric_limits<T>::min());
  r.hi = static_cast<int64_t>(std::numeric_limits<T>::max());
  r.sign = sign;
}

static bool dtypeRange(int dtype, SDtypeRange &r)
{
  switch (dtype)
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        dtypeRangeOf<uint8_t>(r, false);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        dtypeRangeOf<uint8_t>(r, false);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        dtypeRangeOf<int8_t>(r, true);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        dtypeRangeOf<uint16_t>(r, false);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        dtypeRangeOf<int16_t>(r, true);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        dtypeRangeOf<uint32_t>(r, false);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        dtypeRangeOf<int32_t>(r, true);
        return true;
#endif
      default:
        return false;
    }
}

static bool saturateByType(int idtype,
                           int odtype,
                           const void *in,
                           void *out,
                           size_t items,
                           int64_t lo,
                           int64_t hi)
{
  switch (idtype)
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        {
          switch (odtype)
            {
#  ifdef CONFIG_DAWN_DTYPE_BOOL
              case SObjectId::DTYPE_BOOL:
                saturateT<uint8_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
              case SObjectId::DTYPE_UINT8:
                saturateT<uint8_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT8
              case SObjectId::DTYPE_INT8:
                saturateT<uint8_t, int8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT16
              case SObjectId::DTYPE_UINT16:
                saturateT<uint8_t, uint16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT16
              case SObjectId::DTYPE_INT16:
                saturateT<uint8_t, int16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
              case SObjectId::DTYPE_UINT32:
                saturateT<uint8_t, uint32_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT32
              case SObjectId::DTYPE_INT32:
                saturateT<uint8_t, int32_t>(in, out, items, lo, hi);
                return true;
#  endif
              default:
                return false;
            }
        }
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        {
          switch (odtype)
            {
#  ifdef CONFIG_DAWN_DTYPE_BOOL
              case SObjectId::DTYPE_BOOL:
                saturateT<uint8_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
              case SObjectId::DTYPE_UINT8:
                saturateT<uint8_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT8
              case SObjectId::DTYPE_INT8:
                saturateT<uint8_t, int8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT16
              case SObjectId::DTYPE_UINT16:
                saturateT<uint8_t, uint16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT16
              case SObjectId::DTYPE_INT16:
                saturateT<uint8_t, int16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
              case SObjectId::DTYPE_UINT32:
                saturateT<uint8_t, uint32_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT32
              case SObjectId::DTYPE_INT32:
                saturateT<uint8_t, int32_t>(in, out, items, lo, hi);
                return true;
#  endif
              default:
                return false;
            }
        }
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        {
          switch (odtype)
            {
#  ifdef CONFIG_DAWN_DTYPE_BOOL
              case SObjectId::DTYPE_BOOL:
                saturateT<int8_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
              case SObjectId::DTYPE_UINT8:
                saturateT<int8_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT8
              case SObjectId::DTYPE_INT8:
                saturateT<int8_t, int8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT16
              case SObjectId::DTYPE_UINT16:
                saturateT<int8_t, uint16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT16
              case SObjectId::DTYPE_INT16:
                saturateT<int8_t, int16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
              case SObjectId::DTYPE_UINT32:
                saturateT<int8_t, uint32_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT32
              case SObjectId::DTYPE_INT32:
                saturateT<int8_t, int32_t>(in, out, items, lo, hi);
                return true;
#  endif
              default:
                return false;
            }
        }
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        {
          switch (odtype)
            {
#  ifdef CONFIG_DAWN_DTYPE_BOOL
              case SObjectId::DTYPE_BOOL:
                saturateT<uint16_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
              case SObjectId::DTYPE_UINT8:
                saturateT<uint16_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT8
              case SObjectId::DTYPE_INT8:
                saturateT<uint16_t, int8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT16
              case SObjectId::DTYPE_UINT16:
                saturateT<uint16_t, uint16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT16
              case SObjectId::DTYPE_INT16:
                saturateT<uint16_t, int16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
              case SObjectId::DTYPE_UINT32:
                saturateT<uint16_t, uint32_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT32
              case SObjectId::DTYPE_INT32:
                saturateT<uint16_t, int32_t>(in, out, items, lo, hi);
                return true;
#  endif
              default:
                return false;
            }
        }
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        {
          switch (odtype)
            {
#  ifdef CONFIG_DAWN_DTYPE_BOOL
              case SObjectId::DTYPE_BOOL:
                saturateT<int16_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
              case SObjectId::DTYPE_UINT8:
                saturateT<int16_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT8
              case SObjectId::DTYPE_INT8:
                saturateT<int16_t, int8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT16
              case SObjectId::DTYPE_UINT16:
                saturateT<int16_t, uint16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT16
              case SObjectId::DTYPE_INT16:
                saturateT<int16_t, int16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
              case SObjectId::DTYPE_UINT32:
                saturateT<int16_t, uint32_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT32
              case SObjectId::DTYPE_INT32:
                saturateT<int16_t, int32_t>(in, out, items, lo, hi);
                return true;
#  endif
              default:
                return false;
            }
        }
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        {
          switch (odtype)
            {
#  ifdef CONFIG_DAWN_DTYPE_BOOL
              case SObjectId::DTYPE_BOOL:
                saturateT<uint32_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
              case SObjectId::DTYPE_UINT8:
                saturateT<uint32_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT8
              case SObjectId::DTYPE_INT8:
                saturateT<uint32_t, int8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT16
              case SObjectId::DTYPE_UINT16:
                saturateT<uint32_t, uint16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT16
              case SObjectId::DTYPE_INT16:
                saturateT<uint32_t, int16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
              case SObjectId::DTYPE_UINT32:
                saturateT<uint32_t, uint32_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT32
              case SObjectId::DTYPE_INT32:
                saturateT<uint32_t, int32_t>(in, out, items, lo, hi);
                return true;
#  endif
              default:
                return false;
            }
        }
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        {
          switch (odtype)
            {
#  ifdef CONFIG_DAWN_DTYPE_BOOL
              case SObjectId::DTYPE_BOOL:
                saturateT<int32_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
              case SObjectId::DTYPE_UINT8:
                saturateT<int32_t, uint8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT8
              case SObjectId::DTYPE_INT8:
                saturateT<int32_t, int8_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT16
              case SObjectId::DTYPE_UINT16:
                saturateT<int32_t, uint16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT16
              case SObjectId::DTYPE_INT16:
                saturateT<int32_t, int16_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
              case SObjectId::DTYPE_UINT32:
                saturateT<int32_t, uint32_t>(in, out, items, lo, hi);
                return true;
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT32
              case SObjectId::DTYPE_INT32:
                saturateT<int32_t, int32_t>(in, out, items, lo, hi);
                return true;
#  endif
              default:
                return false;
            }
        }
#endif
      default:
        return false;
    }
}

/* Descriptor bound words are two's complement for signed data types and
 * plain values otherwise, matching how limits are stored.
 */

static int64_t decodeBound(uint32_t word, bool sign)
{
  int32_t s;

  if (sign)
    {
      std::memcpy(&s, &word, sizeof(s));
      return static_cast<int64_t>(s);
    }

  return static_cast<int64_t>(word);
}

CProgSaturate::CProgSaturate(CDescObject &desc)
  : CProgCommon(desc)
  , input(nullptr)
  , output(nullptr)
  , inId(0)
  , outId(0)
  , inputData(nullptr)
  , outputData(nullptr)
  , minRaw(0)
  , maxRaw(0)
  , lo(0)
  , hi(0)
  , batch(1)
  , dtype(SObjectId::DTYPE_ANY)
  , outDtype(SObjectId::DTYPE_ANY)
  , hasMin(false)
  , hasMax(false)
  , active(false)
  , registered(false)
{
}

CProgSaturate::~CProgSaturate()
{
  deinit();
}

int CProgSaturate::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  size_t offset;
  size_t ii;

  offset = 0;
  for (ii = 0; ii < desc.getSize(); ii++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_SATURATE)
        {
          DAWNERR("saturate: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_SATURATE_CFG_INPUT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("saturate: INPUT must have 1 entry\n");
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              inId = ids[0].v;
              setObjectMapItem(inId, nullptr);
              break;
            }

          case PROG_SATURATE_CFG_OUTPUT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("saturate: OUTPUT must have 1 entry\n");
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              outId = ids[0].v;
              setObjectMapItem(outId, nullptr);
              break;
            }

          case PROG_SATURATE_CFG_MIN:
            {
              minRaw = item->data[0];
              hasMin = true;
              break;
            }

          case PROG_SATURATE_CFG_MAX:
            {
              maxRaw = item->data[0];
              hasMax = true;
              break;
            }

          default:
            {
              DAWNERR("saturate: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (inId == 0 || outId == 0)
    {
      DAWNERR("saturate: input and output IOs are required\n");
      return -EINVAL;
    }

  if (!hasMin && !hasMax)
    {
      DAWNERR("saturate: at least one bound is required\n");
      return -EINVAL;
    }

  return OK;
}

int CProgSaturate::configure()
{
  return configureDesc(getDesc());
}

int CProgSaturate::resolveBounds()
{
  SDtypeRange r;
  SDtypeRange o;

  if (!dtypeRange(dtype, r) || !dtypeRange(outDtype, o))
    {
      DAWNERR("saturate: unsupported data type %d/%d\n", dtype, outDtype);
      return -ENOTSUP;
    }

  lo = hasMin ? decodeBound(minRaw, r.sign) : r.lo;
  hi = hasMax ? decodeBound(maxRaw, r.sign) : r.hi;

  // Absent bounds default to whichever type is narrower, so a narrowing
  // output cannot be handed a value it has no room for.

  if (!hasMin && o.lo > lo)
    {
      lo = o.lo;
    }

  if (!hasMax && o.hi < hi)
    {
      hi = o.hi;
    }

  // A bound outside either data type range is a descriptor mistake, not
  // something to silently absorb - the clamp would never fire, and the
  // store into a narrower output would truncate.

  if (lo < r.lo || hi > r.hi)
    {
      DAWNERR("saturate: bounds outside the range of data type %d\n", dtype);
      return -ERANGE;
    }

  if (lo < o.lo || hi > o.hi)
    {
      DAWNERR("saturate: bounds outside the range of output type %d\n", outDtype);
      return -ERANGE;
    }

  if (lo > hi)
    {
      DAWNERR("saturate: min above max\n");
      return -EINVAL;
    }

  return OK;
}

int CProgSaturate::init()
{
  size_t dim;
  int ret;

  input = getIO(inId);
  if (!input)
    {
      DAWNERR("saturate: input IO 0x%" PRIx32 " not found\n", inId);
      return -EIO;
    }

  output = getIO(outId);
  if (!output)
    {
      DAWNERR("saturate: output IO 0x%" PRIx32 " not found\n", outId);
      return -EIO;
    }

  if (!input->isRead())
    {
      DAWNERR("saturate: input 0x%" PRIx32 " is not readable\n", inId);
      return -EINVAL;
    }

  dtype = SObjectId::objectIdGetDtype(inId);
  outDtype = SObjectId::objectIdGetDtype(outId);

  ret = resolveBounds();
  if (ret != OK)
    {
      return ret;
    }

  dim = input->getDataDim();
  batch = input->isBatch() ? input->getNotifyBatch() : 1;

  ret = prepareWritableTarget(output, dim, input->isNotify(), batch);
  if (ret != OK)
    {
      DAWNERR("saturate: output target prepare failed %d\n", ret);
      return ret;
    }

  // A notify-driven input hands its buffer to the callback, so the scratch
  // read buffer is only worth its RAM without one.

  if (!input->isNotify())
    {
      inputData = input->ddata_alloc(batch);
      if (inputData == nullptr)
        {
          return -ENOMEM;
        }
    }

  outputData = output->ddata_alloc(batch);
  if (outputData == nullptr)
    {
      return -ENOMEM;
    }

  return OK;
}

int CProgSaturate::deinit()
{
  doStop();

  delete inputData;
  inputData = nullptr;
  delete outputData;
  outputData = nullptr;
  input = nullptr;
  output = nullptr;
  return OK;
}

int CProgSaturate::ioNotifierCb(void *priv, io_ddata_t *data)
{
  CProgSaturate *obj = static_cast<CProgSaturate *>(priv);

  if (obj == nullptr || !obj->active)
    {
      return OK;
    }

  obj->handle(data);
  return OK;
}

int CProgSaturate::doStart()
{
  int ret;

  active = true;

  if (input->isNotify())
    {
      if (!registered)
        {
          ret = input->setNotifier(ioNotifierCb, 0, this);
          if (ret != OK)
            {
              DAWNERR("saturate: setNotifier failed: %d\n", ret);
              return ret;
            }

          registered = true;
        }

      return OK;
    }

  // No notifier on the input: publish once so the output is not left empty.

  ret = input->getData(*inputData, batch);
  if (ret != OK)
    {
      DAWNERR("saturate: getData failed: %d\n", ret);
      return ret;
    }

  handle(inputData);
  return OK;
}

int CProgSaturate::doStop()
{
  if (registered && input != nullptr)
    {
      input->setNotifier(nullptr, 0, nullptr);
      registered = false;
    }

  active = false;
  return OK;
}

bool CProgSaturate::hasThread() const
{
  return false;
}

int CProgSaturate::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  uint32_t oldRaw;
  bool oldHas;
  uint8_t id;
  int ret;

  id = SObjectCfg::objectCfgGetId(objcfg);
  if (id != PROG_SATURATE_CFG_MIN && id != PROG_SATURATE_CFG_MAX)
    {
      return OK;
    }

  if (data == nullptr || len != 1)
    {
      return -EINVAL;
    }

  // Apply, then roll back if the new pair does not hold - a rejected write
  // must not leave the limiter half-updated.

  if (id == PROG_SATURATE_CFG_MIN)
    {
      oldRaw = minRaw;
      oldHas = hasMin;
      minRaw = data[0];
      hasMin = true;
    }
  else
    {
      oldRaw = maxRaw;
      oldHas = hasMax;
      maxRaw = data[0];
      hasMax = true;
    }

  ret = resolveBounds();
  if (ret != OK)
    {
      if (id == PROG_SATURATE_CFG_MIN)
        {
          minRaw = oldRaw;
          hasMin = oldHas;
        }
      else
        {
          maxRaw = oldRaw;
          hasMax = oldHas;
        }

      resolveBounds();
    }

  return ret;
}

void CProgSaturate::handle(io_ddata_t *data)
{
  size_t nbatch;
  size_t nitems;
  size_t flat;
  size_t b;
  bool ok;
  int ret;

  if (data == nullptr || output == nullptr || outputData == nullptr)
    {
      return;
    }

  nbatch = std::min(data->getBatch(), outputData->getBatch());
  nitems = std::min(data->getItems(), outputData->getItems());

  // One pass over the whole buffer when both sides are contiguous - the
  // per-batch path costs a dispatch per sample on a dim-1 stream.

  flat = flatItems(data, nbatch, nitems);
  if (flat != 0 && flatItems(outputData, nbatch, nitems) == flat)
    {
      ok = saturateByType(
        dtype, outDtype, data->getDataPtr(0), outputData->getDataPtr(0), flat, lo, hi);
    }
  else
    {
      // Padded or timestamped batches: only getDataPtr() knows where the
      // elements of each one start.

      ok = true;
      for (b = 0; b < nbatch && ok; b++)
        {
          ok = saturateByType(
            dtype, outDtype, data->getDataPtr(b), outputData->getDataPtr(b), nitems, lo, hi);
        }
    }

  if (!ok)
    {
      DAWNERR("saturate: unsupported data type %d\n", dtype);
      return;
    }

  ret = output->setData(*outputData);
  if (ret != OK)
    {
      DAWNERR("saturate: setData on output failed %d\n", ret);
    }
}
