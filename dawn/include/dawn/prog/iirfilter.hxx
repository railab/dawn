// dawn/include/dawn/prog/iirfilter.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <cstring>
#include <limits>

#include "dawn/io/ddata.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/prog/common.hxx"
#include "dawn/prog/process.hxx"

namespace dawn
{
/**
 * @brief First-order IIR low-pass filter.
 *
 * Uses descriptor-configured rational coefficient alpha = alpha_num /
 * alpha_den and updates output as:
 * y[n] = y[n-1] + alpha * (x[n] - y[n-1])
 */

class CProgIIRFilter : public CProgProcess
{
public:
  enum
  {
    PROG_IIR_CFG_ALPHA_NUM = 2,
    PROG_IIR_CFG_ALPHA_DEN = 3,
  };

  explicit CProgIIRFilter(CDescObject &desc)
    : CProgProcess(desc)
    , alphaNum(1)
    , alphaDen(1)
  {
  }

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "iir";
  }
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_IIR_FILTER, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_IIR_FILTER,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size = 2)
  {
    return CProgIIRFilter::cfgId(false, size, PROG_STATS_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAlphaNum()
  {
    return CProgIIRFilter::cfgId(false, 1, PROG_IIR_CFG_ALPHA_NUM);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdAlphaDen()
  {
    return CProgIIRFilter::cfgId(false, 1, PROG_IIR_CFG_ALPHA_DEN);
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
        DAWNERR("iir: invalid alpha config size %u\n", item->cfgid.s.size);
        return -EINVAL;
      }

    raw = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
    val = SObjectCfg::cfgToU32(raw[0]);

    switch (item->cfgid.s.id)
      {
        case PROG_IIR_CFG_ALPHA_NUM:
          alphaNum = val;
          break;
        case PROG_IIR_CFG_ALPHA_DEN:
          alphaDen = val;
          break;
        default:
          return -ENOTSUP;
      }

    if (alphaDen == 0 || alphaNum > alphaDen)
      {
        DAWNERR("iir: invalid alpha %u/%u\n", alphaNum, alphaDen);
        return -EINVAL;
      }

    return OK;
  }

  void handle(CIOCommon *output,
              io_ddata_t *data,
              io_ddata_t *ioData,
              io_ddata_t *outputData,
              bool &initsample) override
  {
    switch (data->getDtype())
      {
#ifdef CONFIG_DAWN_DTYPE_INT8
        case SObjectId::DTYPE_INT8:
          {
            handleSigned<int8_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT8
        case SObjectId::DTYPE_UINT8:
          {
            handleUnsigned<uint8_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
        case SObjectId::DTYPE_INT16:
          {
            handleSigned<int16_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
        case SObjectId::DTYPE_UINT16:
          {
            handleUnsigned<uint16_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT32
        case SObjectId::DTYPE_INT32:
          {
            handleSigned<int32_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
        case SObjectId::DTYPE_UINT32:
          {
            handleUnsigned<uint32_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_B16
        case SObjectId::DTYPE_B16:
          {
            handleSigned<int32_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_UB16
        case SObjectId::DTYPE_UB16:
          {
            handleUnsigned<uint32_t>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
        case SObjectId::DTYPE_FLOAT:
          {
            handleFloat<float>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
        case SObjectId::DTYPE_DOUBLE:
          {
            handleFloat<double>(output, data, ioData, outputData, initsample);
            break;
          }
#endif

        default:
          {
            DAWNERR("iir: unsupported dtype %d\n", data->getDtype());
            break;
          }
      }
  }

private:
  void handleInitSample(CIOCommon *output,
                        io_ddata_t *data,
                        io_ddata_t *ioData,
                        io_ddata_t *outputData,
                        bool &initsample)
  {
    std::memcpy(ioData->getDataPtr(), data->getDataPtr(), ioData->getDataSize());
    std::memcpy(outputData->getDataPtr(), data->getDataPtr(), outputData->getDataSize());
    output->setData(*outputData);
    initsample = false;
  }

  static int64_t scaleDelta(int64_t diff, uint32_t num, uint32_t den)
  {
    const bool negative = diff < 0;
    const uint64_t magnitude =
      negative ? static_cast<uint64_t>(-diff) : static_cast<uint64_t>(diff);
    const uint64_t quotient = magnitude / den;
    const uint64_t remainder = magnitude % den;
    const uint64_t scaled = quotient * static_cast<uint64_t>(num) +
                            (remainder * static_cast<uint64_t>(num)) / static_cast<uint64_t>(den);

    return negative ? -static_cast<int64_t>(scaled) : static_cast<int64_t>(scaled);
  }

  template<typename T>
  static T clampSigned(int64_t value)
  {
    const int64_t min = static_cast<int64_t>(std::numeric_limits<T>::lowest());
    const int64_t max = static_cast<int64_t>(std::numeric_limits<T>::max());

    if (value < min)
      {
        return std::numeric_limits<T>::lowest();
      }

    if (value > max)
      {
        return std::numeric_limits<T>::max();
      }

    return static_cast<T>(value);
  }

  template<typename T>
  static T clampUnsigned(int64_t value)
  {
    const int64_t max = static_cast<int64_t>(std::numeric_limits<T>::max());

    if (value < 0)
      {
        return 0;
      }

    if (value > max)
      {
        return std::numeric_limits<T>::max();
      }

    return static_cast<T>(value);
  }

  template<typename T>
  void handleSigned(CIOCommon *output,
                    io_ddata_t *data,
                    io_ddata_t *ioData,
                    io_ddata_t *outputData,
                    bool &initsample)
  {
    if (initsample)
      {
        handleInitSample(output, data, ioData, outputData, initsample);
        return;
      }

    std::memcpy(ioData->getDataPtr(), data->getDataPtr(), ioData->getDataSize());

    for (size_t i = 0; i < ioData->getItems(); i++)
      {
        const int64_t x = ioData->get<T>(i);
        int64_t y = outputData->get<T>(i);
        const int64_t diff = x - y;
        y += scaleDelta(diff, alphaNum, alphaDen);
        outputData->get<T>(i) = clampSigned<T>(y);
      }

    output->setData(*outputData);
  }

  template<typename T>
  void handleUnsigned(CIOCommon *output,
                      io_ddata_t *data,
                      io_ddata_t *ioData,
                      io_ddata_t *outputData,
                      bool &initsample)
  {
    if (initsample)
      {
        handleInitSample(output, data, ioData, outputData, initsample);
        return;
      }

    std::memcpy(ioData->getDataPtr(), data->getDataPtr(), ioData->getDataSize());

    for (size_t i = 0; i < ioData->getItems(); i++)
      {
        const int64_t x = ioData->get<T>(i);
        int64_t y = outputData->get<T>(i);
        const int64_t diff = x - y;
        y += scaleDelta(diff, alphaNum, alphaDen);
        outputData->get<T>(i) = clampUnsigned<T>(y);
      }

    output->setData(*outputData);
  }

  template<typename T>
  void handleFloat(CIOCommon *output,
                   io_ddata_t *data,
                   io_ddata_t *ioData,
                   io_ddata_t *outputData,
                   bool &initsample)
  {
    if (initsample)
      {
        handleInitSample(output, data, ioData, outputData, initsample);
        return;
      }

    std::memcpy(ioData->getDataPtr(), data->getDataPtr(), ioData->getDataSize());

    const T alpha = static_cast<T>(alphaNum) / static_cast<T>(alphaDen);

    for (size_t i = 0; i < ioData->getItems(); i++)
      {
        const T x = ioData->get<T>(i);
        T y = outputData->get<T>(i);
        y += (x - y) * alpha;
        outputData->get<T>(i) = y;
      }

    output->setData(*outputData);
  }

  uint32_t alphaNum;
  uint32_t alphaDen;
};
} // Namespace dawn
