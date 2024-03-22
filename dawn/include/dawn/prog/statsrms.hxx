// dawn/include/dawn/prog/statsrms.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <limits>
#include <new>
#include <vector>

#include "dawn/io/ddata.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"
#include "dawn/prog/process.hxx"

namespace dawn
{
/**
 * @brief Running RMS statistics Program.
 *
 * Computes running RMS (root mean square) for incoming samples from a source
 * I/O and publishes the current RMS through a output I/O.
 */

class CProgStatsRms : public CProgProcess
{
public:
  explicit CProgStatsRms(CDescObject &desc)
    : CProgProcess(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "rms";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_STATS_RMS, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_STATS_RMS,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgStatsRms::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }

protected:
  int bindStateAlloc(CIOCommon *src,
                     CIOCommon *output,
                     io_ddata_t *ioData,
                     io_ddata_t *outputData,
                     SBindState **state) override
  {
    (void)output;
    (void)outputData;

    switch (src->getDtype())
      {
#ifdef CONFIG_DAWN_DTYPE_INT8
        case SObjectId::DTYPE_INT8:
          return allocStateInt<int8_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
        case SObjectId::DTYPE_UINT8:
          return allocStateInt<uint8_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
        case SObjectId::DTYPE_INT16:
          return allocStateInt<int16_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
        case SObjectId::DTYPE_UINT16:
          return allocStateInt<uint16_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
        case SObjectId::DTYPE_INT32:
          return allocStateInt<int32_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
        case SObjectId::DTYPE_UINT32:
          return allocStateInt<uint32_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT64
        case SObjectId::DTYPE_INT64:
          return allocStateInt<int64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
        case SObjectId::DTYPE_UINT64:
          return allocStateInt<uint64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
        case SObjectId::DTYPE_FLOAT:
          return allocStateFloat<float>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          return allocStateFloat<double>(ioData->getItems(), state);
#endif
        default:
          {
            DAWNERR("stats rms: unsupported dtype %d\n", src->getDtype());
            return -ENOTSUP;
          }
      }
  }

  void handle(CIOCommon *output,
              io_ddata_t *data,
              io_ddata_t *ioData,
              io_ddata_t *outputData,
              bool &initsample) override
  {
    handleWithState(output, data, ioData, outputData, initsample, nullptr);
  }

  void handleWithState(CIOCommon *output,
                       io_ddata_t *data,
                       io_ddata_t *ioData,
                       io_ddata_t *outputData,
                       bool &initsample,
                       void *state) override
  {
    if (state == nullptr)
      {
        DAWNERR("stats rms: missing state\n");
        return;
      }

    if (initsample)
      {
        static_cast<SBindState *>(state)->reset();
        initsample = false;
      }

    switch (data->getDtype())
      {
#ifdef CONFIG_DAWN_DTYPE_INT8
        case SObjectId::DTYPE_INT8:
          handleInt<int8_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
        case SObjectId::DTYPE_UINT8:
          handleInt<uint8_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
        case SObjectId::DTYPE_INT16:
          handleInt<int16_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
        case SObjectId::DTYPE_UINT16:
          handleInt<uint16_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
        case SObjectId::DTYPE_INT32:
          handleInt<int32_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
        case SObjectId::DTYPE_UINT32:
          handleInt<uint32_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT64
        case SObjectId::DTYPE_INT64:
          handleInt<int64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
        case SObjectId::DTYPE_UINT64:
          handleInt<uint64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
        case SObjectId::DTYPE_FLOAT:
          handleFloat<float>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          handleFloat<double>(output, data, ioData, outputData, state);
          break;
#endif
        default:
          {
            DAWNERR("stats rms: unsupported dtype %d\n", data->getDtype());
            break;
          }
      }
  }

private:
  struct SStateInt final : public SBindState
  {
    uint32_t count = 0;
    uint32_t items = 0;
    std::vector<uint64_t> sumsq;

    void reset() override
    {
      count = 0;
      std::fill(sumsq.begin(), sumsq.end(), 0);
    }
  };

  template<typename T>
  struct SStateFloat final : public SBindState
  {
    uint32_t count = 0;
    uint32_t items = 0;
    std::vector<T> sumsq;

    void reset() override
    {
      count = 0;
      std::fill(sumsq.begin(), sumsq.end(), static_cast<T>(0));
    }
  };

  template<typename T>
  int allocStateInt(size_t items, SBindState **state)
  {
    (void)sizeof(T);

    SStateInt *st = new (std::nothrow) SStateInt();
    if (st == nullptr)
      {
        return -ENOMEM;
      }

    st->items = items;
    st->sumsq.assign(items, 0);
    if (st->sumsq.size() != items)
      {
        delete st;
        return -ENOMEM;
      }

    *state = st;
    return OK;
  }

  template<typename T>
  int allocStateFloat(size_t items, SBindState **state)
  {
    SStateFloat<T> *st = new (std::nothrow) SStateFloat<T>();
    if (st == nullptr)
      {
        return -ENOMEM;
      }

    st->items = items;
    st->sumsq.assign(items, static_cast<T>(0));
    if (st->sumsq.size() != items)
      {
        delete st;
        return -ENOMEM;
      }

    *state = st;
    return OK;
  }

  static uint64_t saturatedAdd(uint64_t a, uint64_t b)
  {
    if (UINT64_MAX - a < b)
      {
        return UINT64_MAX;
      }

    return a + b;
  }

  template<typename T>
  static uint64_t intMagnitude(T v)
  {
    if constexpr (std::numeric_limits<T>::is_signed)
      {
        if (v < 0)
          {
            return static_cast<uint64_t>(0) - static_cast<uint64_t>(v);
          }
      }

    return static_cast<uint64_t>(v);
  }

  static uint64_t squareSat(uint64_t v)
  {
    if (v > UINT32_MAX)
      {
        return UINT64_MAX;
      }

    return v * v;
  }

  static uint64_t isqrt64(uint64_t x)
  {
    uint64_t root = 0;
    uint64_t bit = static_cast<uint64_t>(1) << 62;

    while (bit > x)
      {
        bit >>= 2;
      }

    while (bit != 0)
      {
        if (x >= root + bit)
          {
            x -= root + bit;
            root = (root >> 1) + bit;
          }
        else
          {
            root >>= 1;
          }

        bit >>= 2;
      }

    return root;
  }

  template<typename T>
  static T castIntRms(uint64_t v)
  {
    const uint64_t vmax = static_cast<uint64_t>(std::numeric_limits<T>::max());

    if (v > vmax)
      {
        return std::numeric_limits<T>::max();
      }

    return static_cast<T>(v);
  }

  template<typename T>
  void handleInt(CIOCommon *output,
                 io_ddata_t *data,
                 io_ddata_t *ioData,
                 io_ddata_t *outputData,
                 void *state)
  {
    int ret;
    SStateInt *st = static_cast<SStateInt *>(state);

    std::memcpy(ioData->getDataPtr(), data->getDataPtr(), ioData->getDataSize());

    if (st->items != ioData->getItems())
      {
        DAWNERR("stats rms: item count changed from %u to %zu\n", st->items, ioData->getItems());
        return;
      }

    st->count += 1;

    for (size_t i = 0; i < ioData->getItems(); i++)
      {
        const uint64_t sq = squareSat(intMagnitude(ioData->get<T>(i)));
        const uint64_t sumsq = saturatedAdd(st->sumsq[i], sq);
        const uint64_t meanSq = sumsq / st->count;

        st->sumsq[i] = sumsq;
        outputData->get<T>(i) = castIntRms<T>(isqrt64(meanSq));
      }

    ret = output->setData(*outputData);
    if (ret != OK)
      {
        DAWNERR("failed to set rms value %d\n", ret);
      }
  }

  template<typename T>
  void handleFloat(CIOCommon *output,
                   io_ddata_t *data,
                   io_ddata_t *ioData,
                   io_ddata_t *outputData,
                   void *state)
  {
    int ret;
    SStateFloat<T> *st = static_cast<SStateFloat<T> *>(state);

    std::memcpy(ioData->getDataPtr(), data->getDataPtr(), ioData->getDataSize());

    if (st->items != ioData->getItems())
      {
        DAWNERR("stats rms: item count changed from %u to %zu\n", st->items, ioData->getItems());
        return;
      }

    st->count += 1;

    for (size_t i = 0; i < ioData->getItems(); i++)
      {
        const T x = ioData->get<T>(i);
        const T sumsq = st->sumsq[i] + x * x;

        st->sumsq[i] = sumsq;
        outputData->get<T>(i) = std::sqrt(sumsq / static_cast<T>(st->count));
      }

    ret = output->setData(*outputData);
    if (ret != OK)
      {
        DAWNERR("failed to set rms value %d\n", ret);
      }
  }
};
} // Namespace dawn
