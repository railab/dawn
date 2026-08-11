// dawn/src/prog/saturate.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/saturate.hxx"

#include <algorithm>
#include <limits>
#include <new>
#include <type_traits>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

// How the descriptor words of a bound are decoded

enum EBoundKind
{
  BOUND_UINT, // plain word
  BOUND_INT,  // two's complement word
  BOUND_REAL, // float word
  BOUND_B16,  // b16 fixed-point word
};

// Value range of an integer data type plus its bound word encoding. Real
// types only use the kind: their bounds are float words.

struct SDtypeRange
{
  int64_t lo;
  int64_t hi;
  EBoundKind kind;
};

// Bound word as the input type T: the word is encoded like T

template<typename T>
static T boundAs(uint32_t word)
{
  if constexpr (std::is_floating_point<T>::value)
    {
      return static_cast<T>(SObjectCfg::cfgToF(word));
    }
  else if constexpr (std::is_signed<T>::value)
    {
      return static_cast<T>(SObjectCfg::cfgtoi32(word));
    }
  else
    {
      return static_cast<T>(word);
    }
}

// Clamp in the input type; resolveBounds() proved the bounds fit the output

template<typename TIN, typename TOUT>
static void saturateT(const void *in, void *out, size_t items, uint32_t low, uint32_t highw)
{
  const TIN *src = static_cast<const TIN *>(in);
  TOUT *dst = static_cast<TOUT *>(out);
  const TIN lo = boundAs<TIN>(low);
  const TIN hi = boundAs<TIN>(highw);
  size_t i;

  for (i = 0; i < items; i++)
    {
      TIN v = src[i];

      // NaN fails every compare: integer outputs take lo, real ones pass it

      if (std::is_integral<TOUT>::value ? !(v >= lo) : v < lo)
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

// Range of integer type T with its bound word kind

template<typename T>
static void dtypeRangeOf(SDtypeRange &r, EBoundKind kind)
{
  r.lo = static_cast<int64_t>(std::numeric_limits<T>::lowest());
  r.hi = static_cast<int64_t>(std::numeric_limits<T>::max());
  r.kind = kind;
}

// Range of an enabled data type; false when unsupported or disabled

static bool dtypeRange(int dtype, SDtypeRange &r)
{
  switch (dtype)
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        r.lo = 0;
        r.hi = 1;
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
        r.lo = 0;
        r.hi = 0;
        r.kind = BOUND_REAL;
        return true;
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case SObjectId::DTYPE_DOUBLE:
        r.lo = 0;
        r.hi = 0;
        r.kind = BOUND_REAL;
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

// Clamp TIN samples into the output type odtype; false when unsupported

template<typename TIN>
static bool saturateTo(int odtype,
                       const void *in,
                       void *out,
                       size_t items,
                       uint32_t lo,
                       uint32_t hi)
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

// Clamp samples of input type idtype into odtype; false when unsupported

static bool saturateByType(int idtype,
                           int odtype,
                           const void *in,
                           void *out,
                           size_t items,
                           uint32_t lo,
                           uint32_t hi)
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

// Integer bound word decoded by the input type encoding

static int64_t decodeInt(uint32_t word, EBoundKind kind)
{
  switch (kind)
    {
      case BOUND_INT:
        return SObjectCfg::cfgtoi32(word);
      case BOUND_B16:
        return SObjectCfg::cfgToB16(word);
      default:
        return SObjectCfg::cfgToU32(word);
    }
}

// True when a float stored into an integer type of range [lo, hi] fits it
// (the store truncates toward zero). NaN never fits.

static bool floatFits(float f, int64_t lo, int64_t hi)
{
  int64_t t;

  if (!(f > -9.0e18f && f < 9.0e18f))
    {
      return false;
    }

  t = static_cast<int64_t>(f);
  return t >= lo && t <= hi;
}

// Integer limit as the nearest float that still fits [lo, hi]: a 32-bit max
// rounds up to 2^31 or 2^32, one float step down fits.

static float floatLimit(int64_t v, int64_t lo, int64_t hi)
{
  float f = static_cast<float>(v);

  if (!floatFits(f, lo, hi))
    {
      f = SObjectCfg::cfgToF(SObjectCfg::fToCfg(f) - 1);
    }

  return f;
}

CProgSaturate::CProgSaturate(CDescObject &desc)
  : CProgProcess(desc)
  , minRaw(0)
  , maxRaw(0)
  , hasMin(false)
  , hasMax(false)
{
}

int CProgSaturate::configureExtraCfgItem(const CDescObject &desc,
                                         const SObjectCfg::SObjectCfgItem *item,
                                         size_t &offset)
{
  (void)desc;
  (void)offset;

  if (item->cfgid.s.size != 1)
    {
      DAWNERR("saturate: invalid config size %u\n", item->cfgid.s.size);
      return -EINVAL;
    }

  switch (item->cfgid.s.id)
    {
      case PROG_SATURATE_CFG_MIN:
        {
          minRaw = item->data[0];
          hasMin = true;
          return OK;
        }

      case PROG_SATURATE_CFG_MAX:
        {
          maxRaw = item->data[0];
          hasMax = true;
          return OK;
        }

      default:
        {
          return -ENOTSUP;
        }
    }
}

int CProgSaturate::resolveBounds(SState &st, uint32_t minw, bool hasmin, uint32_t maxw, bool hasmax)
{
  SDtypeRange r;
  SDtypeRange o;
  int64_t loi;
  int64_t hii;
  float lof;
  float hif;

  if (!hasmin && !hasmax)
    {
      DAWNERR("saturate: at least one bound is required\n");
      return -EINVAL;
    }

  if (!dtypeRange(st.dtype, r) || !dtypeRange(st.outDtype, o))
    {
      DAWNERR("saturate: unsupported data type %u/%u\n", st.dtype, st.outDtype);
      return -ENOTSUP;
    }

  // b16 is clamped as raw fixed-point, so it cannot convert to other types

  if ((r.kind == BOUND_B16) != (o.kind == BOUND_B16))
    {
      DAWNERR("saturate: b16 cannot be mixed with type %u/%u\n", st.dtype, st.outDtype);
      return -ENOTSUP;
    }

  // Absent bounds default to whichever type is narrower, so a narrowing
  // output cannot be handed a value it has no room for.

  if (r.kind == BOUND_REAL)
    {
      lof = hasmin ? SObjectCfg::cfgToF(minw) : std::numeric_limits<float>::lowest();
      hif = hasmax ? SObjectCfg::cfgToF(maxw) : std::numeric_limits<float>::max();

      // A bound outside the float range (or NaN) is a descriptor error

      if (!(lof >= std::numeric_limits<float>::lowest() &&
            hif <= std::numeric_limits<float>::max()))
        {
          DAWNERR("saturate: bounds outside the range of data type %u\n", st.dtype);
          return -ERANGE;
        }

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      // double to double is not limited to the float range

      if (st.dtype == SObjectId::DTYPE_DOUBLE && st.outDtype == SObjectId::DTYPE_DOUBLE)
        {
          if (!hasmin)
            {
              lof = -std::numeric_limits<float>::infinity();
            }

          if (!hasmax)
            {
              hif = std::numeric_limits<float>::infinity();
            }
        }
#endif

      if (o.kind != BOUND_REAL)
        {
          if (!hasmin)
            {
              lof = floatLimit(o.lo, o.lo, o.hi);
            }

          if (!hasmax)
            {
              hif = floatLimit(o.hi, o.lo, o.hi);
            }

          if (!floatFits(lof, o.lo, o.hi) || !floatFits(hif, o.lo, o.hi))
            {
              DAWNERR("saturate: bounds outside the range of output type %u\n", st.outDtype);
              return -ERANGE;
            }
        }

      if (lof > hif)
        {
          DAWNERR("saturate: min above max\n");
          return -EINVAL;
        }

      st.lo = SObjectCfg::fToCfg(lof);
      st.hi = SObjectCfg::fToCfg(hif);
      return OK;
    }

  loi = hasmin ? decodeInt(minw, r.kind) : r.lo;
  hii = hasmax ? decodeInt(maxw, r.kind) : r.hi;

  if (o.kind != BOUND_REAL)
    {
      if (!hasmin && o.lo > loi)
        {
          loi = o.lo;
        }

      if (!hasmax && o.hi < hii)
        {
          hii = o.hi;
        }
    }

  // A bound outside either type range is a descriptor error

  if (loi < r.lo || hii > r.hi)
    {
      DAWNERR("saturate: bounds outside the range of data type %u\n", st.dtype);
      return -ERANGE;
    }

  if (o.kind != BOUND_REAL && (loi < o.lo || hii > o.hi))
    {
      DAWNERR("saturate: bounds outside the range of output type %u\n", st.outDtype);
      return -ERANGE;
    }

  if (loi > hii)
    {
      DAWNERR("saturate: min above max\n");
      return -EINVAL;
    }

  // Both fit the input type, so the low 32 bits hold them exactly

  st.lo = static_cast<uint32_t>(loi);
  st.hi = static_cast<uint32_t>(hii);
  return OK;
}

int CProgSaturate::bindStateAlloc(CIOCommon *src,
                                  CIOCommon *output,
                                  io_ddata_t *ioData,
                                  io_ddata_t *outputData,
                                  SBindState **state)
{
  SState *st;
  int ret;

  (void)ioData;
  (void)outputData;

  st = new (std::nothrow) SState();
  if (st == nullptr)
    {
      return -ENOMEM;
    }

  st->dtype = src->getDtype();
  st->outDtype = output->getDtype();

  ret = resolveBounds(*st, minRaw, hasMin, maxRaw, hasMax);
  if (ret != OK)
    {
      delete st;
      return ret;
    }

  states.push_back(st);
  *state = st;
  return OK;
}

int CProgSaturate::deinit()
{
  states.clear();
  return CProgProcess::deinit();
}

int CProgSaturate::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  uint32_t minw = minRaw;
  uint32_t maxw = maxRaw;
  bool hasmin = hasMin;
  bool hasmax = hasMax;
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

  if (id == PROG_SATURATE_CFG_MIN)
    {
      minw = data[0];
      hasmin = true;
    }
  else
    {
      maxw = data[0];
      hasmax = true;
    }

  // Check every binding first, so a rejected write changes nothing

  for (SState *st : states)
    {
      SState tmp;

      tmp.dtype = st->dtype;
      tmp.outDtype = st->outDtype;

      ret = resolveBounds(tmp, minw, hasmin, maxw, hasmax);
      if (ret != OK)
        {
          return ret;
        }
    }

  for (SState *st : states)
    {
      resolveBounds(*st, minw, hasmin, maxw, hasmax);
    }

  minRaw = minw;
  maxRaw = maxw;
  hasMin = hasmin;
  hasMax = hasmax;
  return OK;
}

void CProgSaturate::handle(CIOCommon *output,
                           io_ddata_t *data,
                           io_ddata_t *ioData,
                           io_ddata_t *outputData,
                           bool &initsample)
{
  handleWithState(output, data, ioData, outputData, initsample, nullptr);
}

void CProgSaturate::handleWithState(CIOCommon *output,
                                    io_ddata_t *data,
                                    io_ddata_t *ioData,
                                    io_ddata_t *outputData,
                                    bool &initsample,
                                    void *state)
{
  SState *st = static_cast<SState *>(state);
  size_t nbatch;
  size_t nitems;
  size_t first;
  size_t flat;
  size_t b;
  bool ok;
  int ret;

  (void)ioData;
  initsample = false;

  if (st == nullptr)
    {
      DAWNERR("saturate: missing state\n");
      return;
    }

  // An output holding fewer batches gets the latest samples

  nbatch = std::min(data->getBatch(), outputData->getBatch());
  first = data->getBatch() - nbatch;
  nitems = std::min(data->getItems(), outputData->getItems());

  // One pass when both sides are contiguous, else one per batch

  flat = first == 0 ? contiguousCount(data, nbatch, nitems) : 0;
  if (flat != 0 && contiguousCount(outputData, nbatch, nitems) == flat)
    {
      ok = saturateByType(st->dtype,
                          st->outDtype,
                          data->getDataPtr(0),
                          outputData->getDataPtr(0),
                          flat,
                          st->lo,
                          st->hi);
    }
  else
    {
      ok = true;
      for (b = 0; b < nbatch && ok; b++)
        {
          ok = saturateByType(st->dtype,
                              st->outDtype,
                              data->getDataPtr(first + b),
                              outputData->getDataPtr(b),
                              nitems,
                              st->lo,
                              st->hi);
        }
    }

  if (!ok)
    {
      DAWNERR("saturate: unsupported data type %u\n", st->dtype);
      return;
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
      DAWNERR("saturate: setData on output failed %d\n", ret);
    }
}
