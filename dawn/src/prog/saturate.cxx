// dawn/src/prog/saturate.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/saturate.hxx"

#include <algorithm>
#include <limits>
#include <type_traits>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

/* How the descriptor words of a bound are decoded. */

enum EBoundKind
{
  BOUND_UINT, // plain word
  BOUND_INT,  // two's complement word
  BOUND_REAL, // float word
  BOUND_B16,  // b16 fixed-point word
};

/* Value range of a payload data type plus its bound word encoding. */

struct SDtypeRange
{
  double lo;
  double hi;
  EBoundKind kind;
};

/* Per-sample working type. Integers compare in int32_t unless the input is
 * unsigned 32-bit; comparing in int64_t costs ~20% of the stream rate on a
 * 32-bit core. Real paths use float, widening to double when double is
 * involved or a real input is stored into an integer, so an integer limit
 * that float cannot represent (2^31 - 1) still clamps before the store.
 */

template<typename TIN, typename TOUT>
struct SSatWork
{
  static constexpr bool real =
    std::is_floating_point<TIN>::value || std::is_floating_point<TOUT>::value;
  static constexpr bool wide =
    std::is_same<TIN, double>::value || std::is_same<TOUT, double>::value ||
    (std::is_floating_point<TIN>::value && std::is_integral<TOUT>::value);
  static constexpr bool i64 = sizeof(TIN) >= 4 && !std::is_signed<TIN>::value;

  typedef
    typename std::conditional<real,
                              typename std::conditional<wide, double, float>::type,
                              typename std::conditional<i64, int64_t, int32_t>::type>::type type;
};

/* Clamp in the working type, then store in the output type. resolveBounds()
 * has proven the bounds fit both types, so the store cannot truncate.
 */

template<typename TIN, typename TOUT>
static void saturateT(const void *in, void *out, size_t items, double lod, double hid)
{
  typedef typename SSatWork<TIN, TOUT>::type W;
  const TIN *src = static_cast<const TIN *>(in);
  TOUT *dst = static_cast<TOUT *>(out);
  const W lo = static_cast<W>(lod);
  const W hi = static_cast<W>(hid);
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
static void dtypeRangeOf(SDtypeRange &r, EBoundKind kind)
{
  r.lo = static_cast<double>(std::numeric_limits<T>::lowest());
  r.hi = static_cast<double>(std::numeric_limits<T>::max());
  r.kind = kind;
}

static bool dtypeRange(int dtype, SDtypeRange &r)
{
  switch (dtype)
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        r.lo = 0.0;
        r.hi = 1.0;
        r.kind = BOUND_UINT;
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        dtypeRangeOf<uint8_t>(r, BOUND_UINT);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        dtypeRangeOf<int8_t>(r, BOUND_INT);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        dtypeRangeOf<uint16_t>(r, BOUND_UINT);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        dtypeRangeOf<int16_t>(r, BOUND_INT);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        dtypeRangeOf<uint32_t>(r, BOUND_UINT);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        dtypeRangeOf<int32_t>(r, BOUND_INT);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        dtypeRangeOf<float>(r, BOUND_REAL);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case SObjectId::DTYPE_DOUBLE:
        dtypeRangeOf<double>(r, BOUND_REAL);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
        dtypeRangeOf<int32_t>(r, BOUND_B16);
        return true;
#endif
      default:
        return false;
    }
}

template<typename TIN>
static bool saturateTo(int odtype, const void *in, void *out, size_t items, double lo, double hi)
{
  switch (odtype)
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        saturateT<TIN, uint8_t>(in, out, items, lo, hi);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        saturateT<TIN, uint8_t>(in, out, items, lo, hi);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        saturateT<TIN, int8_t>(in, out, items, lo, hi);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        saturateT<TIN, uint16_t>(in, out, items, lo, hi);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        saturateT<TIN, int16_t>(in, out, items, lo, hi);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        saturateT<TIN, uint32_t>(in, out, items, lo, hi);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        saturateT<TIN, int32_t>(in, out, items, lo, hi);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        saturateT<TIN, float>(in, out, items, lo, hi);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case SObjectId::DTYPE_DOUBLE:
        saturateT<TIN, double>(in, out, items, lo, hi);
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
        saturateT<TIN, int32_t>(in, out, items, lo, hi);
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
                           double lo,
                           double hi)
{
  switch (idtype)
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        return saturateTo<uint8_t>(odtype, in, out, items, lo, hi);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        return saturateTo<uint8_t>(odtype, in, out, items, lo, hi);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        return saturateTo<int8_t>(odtype, in, out, items, lo, hi);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        return saturateTo<uint16_t>(odtype, in, out, items, lo, hi);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        return saturateTo<int16_t>(odtype, in, out, items, lo, hi);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        return saturateTo<uint32_t>(odtype, in, out, items, lo, hi);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        return saturateTo<int32_t>(odtype, in, out, items, lo, hi);
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        return saturateTo<float>(odtype, in, out, items, lo, hi);
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case SObjectId::DTYPE_DOUBLE:
        return saturateTo<double>(odtype, in, out, items, lo, hi);
#endif
#ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
        return saturateTo<int32_t>(odtype, in, out, items, lo, hi);
#endif
      default:
        return false;
    }
}

/* Descriptor bound words follow the input data type: two's complement for
 * signed integers, a float word for real types, a b16 word for b16.
 */

static double decodeBound(uint32_t word, EBoundKind kind)
{
  switch (kind)
    {
      case BOUND_INT:
        return SObjectCfg::cfgtoi32(word);
      case BOUND_REAL:
        return SObjectCfg::cfgToF(word);
      case BOUND_B16:
        return SObjectCfg::cfgToB16(word);
      default:
        return SObjectCfg::cfgToU32(word);
    }
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

  // b16 is clamped as raw fixed-point, so it cannot convert to other types

  if ((r.kind == BOUND_B16) != (o.kind == BOUND_B16))
    {
      DAWNERR("saturate: b16 cannot be mixed with type %d/%d\n", dtype, outDtype);
      return -ENOTSUP;
    }

  lo = hasMin ? decodeBound(minRaw, r.kind) : r.lo;
  hi = hasMax ? decodeBound(maxRaw, r.kind) : r.hi;

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

  // A bound outside either data type range (or NaN) is a descriptor
  // mistake, not something to silently absorb - the clamp would never fire,
  // and the store into a narrower output would truncate.

  if (!(lo >= r.lo && hi <= r.hi))
    {
      DAWNERR("saturate: bounds outside the range of data type %d\n", dtype);
      return -ERANGE;
    }

  if (!(lo >= o.lo && hi <= o.hi))
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

  std::lock_guard<std::mutex> lock(boundsLock);

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
  double lo;
  double hi;
  bool ok;
  int ret;

  if (data == nullptr || output == nullptr || outputData == nullptr)
    {
      return;
    }

  // One consistent bound pair per buffer, not per sample

  {
    std::lock_guard<std::mutex> lock(boundsLock);
    lo = this->lo;
    hi = this->hi;
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
