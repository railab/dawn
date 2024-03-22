// dawn/include/dawn/prog/threshold.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <cstring>
#include <inttypes.h>
#include <map>
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

  int trigger(uint8_t cmd) override
  {
    if (cmd == CMD_RESET)
      {
        state.clear();
      }

    return CProgProcess::trigger(cmd);
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

  void handle(CIOCommon *output,
              io_ddata_t *data,
              io_ddata_t *ioData,
              io_ddata_t *outputData,
              bool &initsample) override
  {
    (void)initsample;

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
            handleTyped<float>(
              output, data, ioData, outputData, SObjectCfg::cfgToF(low), SObjectCfg::cfgToF(high));
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          {
            handleTyped<double>(output,
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

  virtual bool validateOutputDtype(uint8_t inputDtype, uint8_t outputDtype) const = 0;
  virtual int emitOutput(CIOCommon *output,
                         uint8_t inputDtype,
                         size_t items,
                         io_ddata_t *ioData,
                         io_ddata_t *outputData) = 0;

private:
  struct SState
  {
    size_t items = 0;
    std::vector<uint8_t> last;
  };

  template<typename T>
  bool evaluate(T x, T lowT, T highT, uint8_t prev) const
  {
    switch (mode)
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

  template<typename T>
  void handleTyped(CIOCommon *output,
                   io_ddata_t *data,
                   io_ddata_t *ioData,
                   io_ddata_t *outputData,
                   T lowT,
                   T highT)
  {
    int ret;

    std::memcpy(ioData->getDataPtr(), data->getDataPtr(), ioData->getDataSize());

    const SObjectId::ObjectId outputId = output->getIdV();
    SState &st = state[outputId];
    const size_t items = ioData->getItems();

    if (st.items != items || st.last.size() != items)
      {
        st.items = items;
        st.last.assign(items, 0);
      }

    for (size_t i = 0; i < items; i++)
      {
        const uint8_t prev = st.last[i];
        const T x = ioData->get<T>(i);
        const bool out = evaluate(x, lowT, highT, prev);

        st.last[i] = out ? 1 : 0;
      }

    this->alerts = st.last;
    ret = emitOutput(output, data->getDtype(), items, ioData, outputData);
    if (ret != OK)
      {
        DAWNERR("threshold: emit failed %d\n", ret);
      }
  }

protected:
  uint32_t mode;
  SObjectCfg::ObjectCfgData_t low;
  SObjectCfg::ObjectCfgData_t high;
  std::vector<uint8_t> alerts;

private:
  std::map<SObjectId::ObjectId, SState> state;
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
                 io_ddata_t *ioData,
                 io_ddata_t *outputData) override
  {
    (void)inputDtype;
    (void)ioData;

    for (size_t i = 0; i < items; i++)
      {
        outputData->get<uint8_t>(i) = this->alerts[i];
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
                 io_ddata_t *ioData,
                 io_ddata_t *outputData) override
  {
    switch (inputDtype)
      {
#ifdef CONFIG_DAWN_DTYPE_INT8
        case SObjectId::DTYPE_INT8:
          {
            return emitOutputTyped<int8_t>(output, items, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT8
        case SObjectId::DTYPE_UINT8:
          {
            return emitOutputTyped<uint8_t>(output, items, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
        case SObjectId::DTYPE_INT16:
          {
            return emitOutputTyped<int16_t>(output, items, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
        case SObjectId::DTYPE_UINT16:
          {
            return emitOutputTyped<uint16_t>(output, items, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT32
        case SObjectId::DTYPE_INT32:
          {
            return emitOutputTyped<int32_t>(output, items, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
        case SObjectId::DTYPE_UINT32:
          {
            return emitOutputTyped<uint32_t>(output, items, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT64
        case SObjectId::DTYPE_INT64:
          {
            return emitOutputTyped<int64_t>(output, items, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT64
        case SObjectId::DTYPE_UINT64:
          {
            return emitOutputTyped<uint64_t>(output, items, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
        case SObjectId::DTYPE_FLOAT:
          {
            return emitOutputTyped<float>(output, items, ioData, outputData);
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          {
            return emitOutputTyped<double>(output, items, ioData, outputData);
          }
#endif

        default:
          {
            return -ENOTSUP;
          }
      }
  }

private:
  template<typename T>
  int emitOutputTyped(CIOCommon *output, size_t items, io_ddata_t *ioData, io_ddata_t *outputData)
  {
    for (size_t i = 0; i < items; i++)
      {
        if (this->alerts[i] != 0)
          {
            outputData->get<T>(i) = ioData->get<T>(i);
          }
        else
          {
            outputData->get<T>(i) = static_cast<T>(0);
          }
      }

    return output->setData(*outputData);
  }
};
} // Namespace dawn
