// dawn/include/dawn/prog/threshold.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <inttypes.h>
#include <new>
#include <vector>

#include "dawn/io/ddata.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"
#include "dawn/prog/process.hxx"

namespace dawn
{
/** @brief Shared threshold logic for derived threshold programs. */

class CProgThresholdBase : public CProgProcess
{
public:
  enum
  {
    PROG_THRESHOLD_CFG_MODE = 2,
    PROG_THRESHOLD_CFG_LOW = 3,
    PROG_THRESHOLD_CFG_HIGH = 4,
  };

  enum
  {
    MODE_ABOVE = 0,
    MODE_BELOW = 1,
    MODE_HYSTERESIS = 2,
    MODE_WINDOW = 3,
  };

  explicit CProgThresholdBase(CDescObject &desc)
    : CProgProcess(desc)
    , mode(MODE_ABOVE)
    , low(0)
    , high(0)
  {
  }

protected:
  int configureExtraCfgItem(const CDescObject &desc,
                            const SObjectCfg::SObjectCfgItem *item,
                            size_t &offset) override
  {
    const SObjectCfg::ObjectCfgData_t *raw;
    uint32_t val;

    (void)desc;
    (void)offset;

    if (item->cfgid.s.size != 1)
      {
        DAWNERR("threshold: invalid config size %u\n", item->cfgid.s.size);
        return -EINVAL;
      }

    raw = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
    val = SObjectCfg::cfgToU32(raw[0]);

    switch (item->cfgid.s.id)
      {
        case PROG_THRESHOLD_CFG_MODE:
          {
            if (val > MODE_WINDOW)
              {
                DAWNERR("threshold: invalid mode %" PRIu32 "\n", val);
                return -EINVAL;
              }

            mode = val;
            break;
          }

        case PROG_THRESHOLD_CFG_LOW:
          {
            low = raw[0];
            break;
          }

        case PROG_THRESHOLD_CFG_HIGH:
          {
            high = raw[0];
            break;
          }

        default:
          {
            return -ENOTSUP;
          }
      }

    return OK;
  }

  int bindStateAlloc(CIOCommon *src,
                     CIOCommon *output,
                     io_ddata_t *ioData,
                     io_ddata_t *outputData,
                     SBindState **state) override
  {
    const size_t items = ioData->getItems();
    const size_t batches = ioData->getBatch();
    SState *st;

    (void)src;
    (void)output;
    (void)outputData;

    st = new (std::nothrow) SState();
    if (st == nullptr)
      {
        return -ENOMEM;
      }

    st->last.assign(items, 0);
    st->alerts.assign(items * batches, 0);
    if (st->last.size() != items || st->alerts.size() != items * batches)
      {
        delete st;
        return -ENOMEM;
      }

    *state = st;
    return OK;
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
    SState *st = static_cast<SState *>(state);

    if (st == nullptr)
      {
        DAWNERR("threshold: missing state\n");
        return;
      }

    // Reset is deferred to the callback so it never races with evaluation

    if (initsample)
      {
        std::fill(st->last.begin(), st->last.end(), 0);
        initsample = false;
      }

    if (!validateOutputDtype(data->getDtype(), outputData->getDtype()))
      {
        DAWNERR("threshold: invalid output dtype %d for input dtype %d\n",
                outputData->getDtype(),
                data->getDtype());
        return;
      }

    switch (data->getDtype())
      {
#ifdef CONFIG_DAWN_DTYPE_INT8
        case SObjectId::DTYPE_INT8:
          {
            handleTyped<int8_t>(output,
                                st,
                                data,
                                ioData,
                                outputData,
                                static_cast<int8_t>(low),
                                static_cast<int8_t>(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT8
        case SObjectId::DTYPE_UINT8:
          {
            handleTyped<uint8_t>(output,
                                 st,
                                 data,
                                 ioData,
                                 outputData,
                                 static_cast<uint8_t>(low),
                                 static_cast<uint8_t>(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
        case SObjectId::DTYPE_INT16:
          {
            handleTyped<int16_t>(output,
                                 st,
                                 data,
                                 ioData,
                                 outputData,
                                 static_cast<int16_t>(low),
                                 static_cast<int16_t>(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
        case SObjectId::DTYPE_UINT16:
          {
            handleTyped<uint16_t>(output,
                                  st,
                                  data,
                                  ioData,
                                  outputData,
                                  static_cast<uint16_t>(low),
                                  static_cast<uint16_t>(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT32
        case SObjectId::DTYPE_INT32:
          {
            handleTyped<int32_t>(output,
                                 st,
                                 data,
                                 ioData,
                                 outputData,
                                 SObjectCfg::cfgtoi32(low),
                                 SObjectCfg::cfgtoi32(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
        case SObjectId::DTYPE_UINT32:
          {
            handleTyped<uint32_t>(output,
                                  st,
                                  data,
                                  ioData,
                                  outputData,
                                  SObjectCfg::cfgToU32(low),
                                  SObjectCfg::cfgToU32(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT64
        case SObjectId::DTYPE_INT64:
          {
            handleTyped<int64_t>(output,
                                 st,
                                 data,
                                 ioData,
                                 outputData,
                                 static_cast<int64_t>(low),
                                 static_cast<int64_t>(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT64
        case SObjectId::DTYPE_UINT64:
          {
            handleTyped<uint64_t>(output,
                                  st,
                                  data,
                                  ioData,
                                  outputData,
                                  static_cast<uint64_t>(low),
                                  static_cast<uint64_t>(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
        case SObjectId::DTYPE_FLOAT:
          {
            handleTyped<float>(output,
                               st,
                               data,
                               ioData,
                               outputData,
                               SObjectCfg::cfgToF(low),
                               SObjectCfg::cfgToF(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          {
            handleTyped<double>(output,
                                st,
                                data,
                                ioData,
                                outputData,
                                static_cast<double>(SObjectCfg::cfgToF(low)),
                                static_cast<double>(SObjectCfg::cfgToF(high)));
            break;
          }
#endif

        default:
          {
            DAWNERR("threshold: unsupported dtype %d\n", data->getDtype());
            break;
          }
      }
  }

  // Every sample of a batched source is evaluated

  bool isBatchAware() const override
  {
    return true;
  }

  virtual bool validateOutputDtype(uint8_t inputDtype, uint8_t outputDtype) const = 0;

  /**
   * @brief Publish the evaluated batches to the output.
   *
   * @param[in] output Output IO.
   * @param[in] inputDtype Source data type.
   * @param[in] items Elements per batch.
   * @param[in] first First source batch to publish.
   * @param[in] batches Number of batches to publish.
   * @param[in] alerts Alert flags, items per source batch.
   * @param[in] ioData Source samples.
   * @param[in] outputData Output buffer.
   * @return OK on success, or error code from the output.
   */

  virtual int emitOutput(CIOCommon *output,
                         uint8_t inputDtype,
                         size_t items,
                         size_t first,
                         size_t batches,
                         const uint8_t *alerts,
                         io_ddata_t *ioData,
                         io_ddata_t *outputData) = 0;

private:
  /** @brief Per-binding state, sized once in bindStateAlloc(). */

  struct SState final : public SBindState
  {
    std::vector<uint8_t> last;
    std::vector<uint8_t> alerts;
  };

  /** @brief Evaluate one sample in mode M given the previous alert state. */

  template<uint32_t M, typename T>
  static bool evalMode(T x, T lowT, T highT, uint8_t prev)
  {
    switch (M)
      {
        case MODE_ABOVE:
          return x >= highT;

        case MODE_BELOW:
          return x <= lowT;

        case MODE_WINDOW:
          return x >= lowT && x <= highT;

        case MODE_HYSTERESIS:
          if (x >= highT)
            {
              return true;
            }

          if (x <= lowT)
            {
              return false;
            }

          return prev != 0;

        default:
          return false;
      }
  }

  /**
   * @brief Evaluate nb batches of items samples in mode M.
   *
   * Samples are walked in order so the state in last carries across
   * batches; alerts get one flag per sample.
   */

  template<uint32_t M, typename T>
  static void
  evalLoop(const T *x, size_t nb, size_t items, T lowT, T highT, uint8_t *last, uint8_t *alerts)
  {
    size_t b;
    size_t i;

    for (b = 0; b < nb; b++)
      {
        for (i = 0; i < items; i++)
          {
            last[i] = evalMode<M>(x[b * items + i], lowT, highT, last[i]) ? 1 : 0;
            alerts[b * items + i] = last[i];
          }
      }
  }

  /** @brief Evaluate a run of samples, resolving the mode once per run. */

  template<typename T>
  void evalRun(const T *x, size_t nb, size_t items, T lowT, T highT, uint8_t *last, uint8_t *alerts)
    const
  {
    switch (mode)
      {
        case MODE_ABOVE:
          evalLoop<MODE_ABOVE>(x, nb, items, lowT, highT, last, alerts);
          break;

        case MODE_BELOW:
          evalLoop<MODE_BELOW>(x, nb, items, lowT, highT, last, alerts);
          break;

        case MODE_WINDOW:
          evalLoop<MODE_WINDOW>(x, nb, items, lowT, highT, last, alerts);
          break;

        case MODE_HYSTERESIS:
          evalLoop<MODE_HYSTERESIS>(x, nb, items, lowT, highT, last, alerts);
          break;

        default:
          std::memset(alerts, 0, nb * items);
          break;
      }
  }

  /** @brief Copy, evaluate and publish one source buffer of type T. */

  template<typename T>
  void handleTyped(CIOCommon *output,
                   SState *st,
                   io_ddata_t *data,
                   io_ddata_t *ioData,
                   io_ddata_t *outputData,
                   T lowT,
                   T highT)
  {
    const size_t items = ioData->getItems();
    const size_t batches = std::min(data->getBatch(), ioData->getBatch());
    const size_t outBatches = std::min(batches, outputData->getBatch());
    const size_t first = batches - outBatches;
    const size_t flat = contiguousCount(ioData, batches, items);
    uint8_t *alerts = st->alerts.data();
    size_t b;
    int ret;

    // One pass when both sides are contiguous, else one per batch

    if (flat != 0 && contiguousCount(data, batches, items) == flat)
      {
        std::memcpy(ioData->getDataPtr(0), data->getDataPtr(0), flat * sizeof(T));
      }
    else
      {
        for (b = 0; b < batches; b++)
          {
            std::memcpy(ioData->getDataPtr(b), data->getDataPtr(b), ioData->getDataSize());
          }
      }

    // An output holding fewer batches gets the latest samples

    if (outputData->hasTimestamp() && data->hasTimestamp())
      {
        for (b = 0; b < outBatches; b++)
          {
            outputData->getTs(b) = data->getTs(first + b);
          }
      }

    // Samples are evaluated in order so hysteresis carries across the batch

    if (flat != 0)
      {
        evalRun(static_cast<const T *>(ioData->getDataPtr(0)),
                batches,
                items,
                lowT,
                highT,
                st->last.data(),
                alerts);
      }
    else
      {
        for (b = 0; b < batches; b++)
          {
            evalRun(static_cast<const T *>(ioData->getDataPtr(b)),
                    1,
                    items,
                    lowT,
                    highT,
                    st->last.data(),
                    alerts + b * items);
          }
      }

    ret =
      emitOutput(output, data->getDtype(), items, first, outBatches, alerts, ioData, outputData);
    if (ret != OK)
      {
        DAWNERR("threshold: emit failed %d\n", ret);
      }
  }

protected:
  uint32_t mode;
  SObjectCfg::ObjectCfgData_t low;
  SObjectCfg::ObjectCfgData_t high;
};

/**
 * @brief Threshold comparator returning boolean alert output.
 */

class CProgThreshold : public CProgThresholdBase
{
public:
  explicit CProgThreshold(CDescObject &desc)
    : CProgThresholdBase(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "threshold";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_THRESHOLD, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_THRESHOLD,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgThreshold::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdMode()
  {
    return CProgThreshold::cfgId(false, 1, PROG_THRESHOLD_CFG_MODE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdLow()
  {
    return CProgThreshold::cfgId(false, 1, PROG_THRESHOLD_CFG_LOW);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdHigh()
  {
    return CProgThreshold::cfgId(false, 1, PROG_THRESHOLD_CFG_HIGH);
  }

protected:
  bool validateOutputDtype(uint8_t inputDtype, uint8_t outputDtype) const override
  {
    (void)inputDtype;
    return outputDtype == SObjectId::DTYPE_BOOL;
  }

  int emitOutput(CIOCommon *output,
                 uint8_t inputDtype,
                 size_t items,
                 size_t first,
                 size_t batches,
                 const uint8_t *alerts,
                 io_ddata_t *ioData,
                 io_ddata_t *outputData) override
  {
    (void)inputDtype;
    (void)ioData;

    if (contiguousCount(outputData, batches, items) != 0)
      {
        std::memcpy(outputData->getDataPtr(0), alerts + first * items, batches * items);
        return output->setData(*outputData);
      }

    for (size_t b = 0; b < batches; b++)
      {
        for (size_t i = 0; i < items; i++)
          {
            outputData->get<uint8_t>(i, b) = alerts[(first + b) * items + i];
          }
      }

    return output->setData(*outputData);
  }
};

/**
 * @brief Threshold comparator returning gated source values.
 */

class CProgThresholdValue : public CProgThresholdBase
{
public:
  explicit CProgThresholdValue(CDescObject &desc)
    : CProgThresholdBase(desc)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "thresholdvalue";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(SObjectId::OBJTYPE_PROG,
                               CProgCommon::PROG_CLASS_THRESHOLD_VALUE,
                               SObjectId::DTYPE_ANY,
                               0,
                               inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_THRESHOLD_VALUE,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgThresholdValue::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdMode()
  {
    return CProgThresholdValue::cfgId(false, 1, PROG_THRESHOLD_CFG_MODE);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdLow()
  {
    return CProgThresholdValue::cfgId(false, 1, PROG_THRESHOLD_CFG_LOW);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdHigh()
  {
    return CProgThresholdValue::cfgId(false, 1, PROG_THRESHOLD_CFG_HIGH);
  }

protected:
  bool validateOutputDtype(uint8_t inputDtype, uint8_t outputDtype) const override
  {
    return outputDtype == inputDtype;
  }

  int emitOutput(CIOCommon *output,
                 uint8_t inputDtype,
                 size_t items,
                 size_t first,
                 size_t batches,
                 const uint8_t *alerts,
                 io_ddata_t *ioData,
                 io_ddata_t *outputData) override
  {
    switch (inputDtype)
      {
#ifdef CONFIG_DAWN_DTYPE_INT8
        case SObjectId::DTYPE_INT8:
          {
            return emitOutputTyped<int8_t>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT8
        case SObjectId::DTYPE_UINT8:
          {
            return emitOutputTyped<uint8_t>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
        case SObjectId::DTYPE_INT16:
          {
            return emitOutputTyped<int16_t>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
        case SObjectId::DTYPE_UINT16:
          {
            return emitOutputTyped<uint16_t>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT32
        case SObjectId::DTYPE_INT32:
          {
            return emitOutputTyped<int32_t>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
        case SObjectId::DTYPE_UINT32:
          {
            return emitOutputTyped<uint32_t>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT64
        case SObjectId::DTYPE_INT64:
          {
            return emitOutputTyped<int64_t>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT64
        case SObjectId::DTYPE_UINT64:
          {
            return emitOutputTyped<uint64_t>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
        case SObjectId::DTYPE_FLOAT:
          {
            return emitOutputTyped<float>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          {
            return emitOutputTyped<double>(
              output, items, first, batches, alerts, ioData, outputData);
          }
#endif

        default:
          {
            return -ENOTSUP;
          }
      }
  }

private:
  /** @brief Publish source samples that raised an alert, 0 elsewhere. */

  template<typename T>
  int emitOutputTyped(CIOCommon *output,
                      size_t items,
                      size_t first,
                      size_t batches,
                      const uint8_t *alerts,
                      io_ddata_t *ioData,
                      io_ddata_t *outputData)
  {
    const size_t n = batches * items;

    if (contiguousCount(outputData, batches, items) == n &&
        contiguousCount(ioData, first + batches, items) != 0)
      {
        const T *in = static_cast<const T *>(ioData->getDataPtr(0)) + first * items;
        const uint8_t *pass = alerts + first * items;
        T *out = static_cast<T *>(outputData->getDataPtr(0));

        for (size_t k = 0; k < n; k++)
          {
            out[k] = pass[k] != 0 ? in[k] : static_cast<T>(0);
          }

        return output->setData(*outputData);
      }

    for (size_t b = 0; b < batches; b++)
      {
        for (size_t i = 0; i < items; i++)
          {
            const size_t src = first + b;
            const bool pass = alerts[src * items + i] != 0;

            outputData->get<T>(i, b) = pass ? ioData->get<T>(i, src) : static_cast<T>(0);
          }
      }

    return output->setData(*outputData);
  }
};
} // Namespace dawn
