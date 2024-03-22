// dawn/include/dawn/prog/movingavg.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
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
 * @brief Notify-driven sliding window moving average filter.
 *
 * Computes an arithmetic mean over the last @c window samples for each input
 * element and publishes the filtered result through a output IO.
 */

class CProgMovingAverage : public CProgProcess
{
public:
  enum
  {
    PROG_MOVAVG_CFG_WINDOW = 2,
  };

  explicit CProgMovingAverage(CDescObject &desc)
    : CProgProcess(desc)
    , window(1)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "movingavg";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_MOVING_AVG, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_MOVING_AVG,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgMovingAverage::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdWindow()
  {
    return CProgMovingAverage::cfgId(false, 1, PROG_MOVAVG_CFG_WINDOW);
  }

protected:
  int configureExtraCfgItem(const CDescObject &desc,
                            const SObjectCfg::SObjectCfgItem *item,
                            size_t &offset) override
  {
    (void)desc;
    (void)offset;

    switch (item->cfgid.s.id)
      {
        case PROG_MOVAVG_CFG_WINDOW:
          {
            const SObjectCfg::ObjectCfgData_t *raw;

            if (item->cfgid.s.size != 1)
              {
                DAWNERR("movingavg: invalid WINDOW size %u\n", item->cfgid.s.size);
                return -EINVAL;
              }

            raw = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
            window = SObjectCfg::cfgToU32(raw[0]);
            if (window == 0)
              {
                DAWNERR("movingavg: window must be > 0\n");
                return -EINVAL;
              }

            return OK;
          }

        default:
          {
            return -ENOTSUP;
          }
      }
  }

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
          return allocState<int8_t, int64_t, int64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
        case SObjectId::DTYPE_UINT8:
          return allocState<uint8_t, uint64_t, uint64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
        case SObjectId::DTYPE_INT16:
          return allocState<int16_t, int64_t, int64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
        case SObjectId::DTYPE_UINT16:
          return allocState<uint16_t, uint64_t, uint64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
        case SObjectId::DTYPE_INT32:
          return allocState<int32_t, int64_t, int64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
        case SObjectId::DTYPE_UINT32:
          return allocState<uint32_t, uint64_t, uint64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_INT64
        case SObjectId::DTYPE_INT64:
          return allocState<int64_t, int64_t, int64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
        case SObjectId::DTYPE_UINT64:
          return allocState<uint64_t, uint64_t, uint64_t>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
        case SObjectId::DTYPE_FLOAT:
          return allocState<float, float, float>(ioData->getItems(), state);
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          return allocState<double, double, double>(ioData->getItems(), state);
#endif
        default:
          {
            DAWNERR("movingavg: unsupported dtype %d\n", src->getDtype());
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
        DAWNERR("movingavg: missing state\n");
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
          handleTyped<int8_t, int64_t, int64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT8
        case SObjectId::DTYPE_UINT8:
          handleTyped<uint8_t, uint64_t, uint64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT16
        case SObjectId::DTYPE_INT16:
          handleTyped<int16_t, int64_t, int64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT16
        case SObjectId::DTYPE_UINT16:
          handleTyped<uint16_t, uint64_t, uint64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT32
        case SObjectId::DTYPE_INT32:
          handleTyped<int32_t, int64_t, int64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT32
        case SObjectId::DTYPE_UINT32:
          handleTyped<uint32_t, uint64_t, uint64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_INT64
        case SObjectId::DTYPE_INT64:
          handleTyped<int64_t, int64_t, int64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_UINT64
        case SObjectId::DTYPE_UINT64:
          handleTyped<uint64_t, uint64_t, uint64_t>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_FLOAT
        case SObjectId::DTYPE_FLOAT:
          handleTyped<float, float, float>(output, data, ioData, outputData, state);
          break;
#endif
#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          handleTyped<double, double, double>(output, data, ioData, outputData, state);
          break;
#endif
        default:
          {
            DAWNERR("movingavg: unsupported dtype %d\n", data->getDtype());
            break;
          }
      }
  }

private:
  template<typename T, typename HistT, typename SumT>
  struct SState final : public SBindState
  {
    uint32_t count = 0;
    uint32_t pos = 0;
    uint32_t items = 0;
    std::vector<HistT> hist;
    std::vector<SumT> sum;

    void reset() override
    {
      count = 0;
      pos = 0;
      std::fill(hist.begin(), hist.end(), static_cast<HistT>(0));
      std::fill(sum.begin(), sum.end(), static_cast<SumT>(0));
    }
  };

  template<typename T, typename HistT, typename SumT>
  int allocState(size_t items, SBindState **state)
  {
    SState<T, HistT, SumT> *st;

    st = new (std::nothrow) SState<T, HistT, SumT>();
    if (st == nullptr)
      {
        return -ENOMEM;
      }

    st->items = items;
    st->hist.assign(items * window, static_cast<HistT>(0));
    st->sum.assign(items, static_cast<SumT>(0));
    if (st->hist.size() != items * window || st->sum.size() != items)
      {
        delete st;
        return -ENOMEM;
      }

    *state = st;
    return OK;
  }

  template<typename T, typename HistT, typename SumT>
  void handleTyped(CIOCommon *output,
                   io_ddata_t *data,
                   io_ddata_t *ioData,
                   io_ddata_t *outputData,
                   void *state)
  {
    SState<T, HistT, SumT> *st = static_cast<SState<T, HistT, SumT> *>(state);
    const size_t items = ioData->getItems();
    const uint32_t used = (st->count < window) ? st->count : window;

    std::memcpy(ioData->getDataPtr(), data->getDataPtr(), ioData->getDataSize());

    if (st->items != items)
      {
        DAWNERR("movingavg: item count changed from %u to %zu\n", st->items, items);
        return;
      }

    for (size_t i = 0; i < items; i++)
      {
        const size_t hidx = i * window + st->pos;
        const HistT x = static_cast<HistT>(ioData->get<T>(i));

        if (used == window)
          {
            st->sum[i] -= st->hist[hidx];
          }

        st->hist[hidx] = x;
        st->sum[i] += x;
      }

    if (st->count < window)
      {
        st->count++;
      }

    st->pos = (st->pos + 1) % window;

    for (size_t i = 0; i < items; i++)
      {
        outputData->get<T>(i) = static_cast<T>(st->sum[i] / static_cast<SumT>(st->count));
      }

    output->setData(*outputData);
  }

  uint32_t window;
};
} // Namespace dawn
