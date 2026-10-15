// dawn/src/prog/saturate.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/saturate.hxx"

#include <algorithm>
#include <limits>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

// Integer bound word, or the type limit when the bound is not set

template<typename T>
static T bound(uint32_t raw, bool set, T limit)
{
  return set ? static_cast<T>(static_cast<int32_t>(raw)) : limit;
}

// Real bounds travel as one float word

static float bound(uint32_t raw, bool set, float limit)
{
  return set ? SObjectCfg::cfgToF(raw) : limit;
}

static double bound(uint32_t raw, bool set, double limit)
{
  return set ? static_cast<double>(SObjectCfg::cfgToF(raw)) : limit;
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

int CProgSaturate::onSetObjConfig(SObjectCfg::ObjectCfgId objcfg, uint32_t *data, size_t len)
{
  if (data == nullptr || len != 1)
    {
      return -EINVAL;
    }

  switch (SObjectCfg::objectCfgGetId(objcfg))
    {
      case PROG_SATURATE_CFG_MIN:
        {
          minRaw = data[0];
          return OK;
        }

      case PROG_SATURATE_CFG_MAX:
        {
          maxRaw = data[0];
          return OK;
        }

      default:
        {
          return OK;
        }
    }
}

template<typename T>
void CProgSaturate::clamp(io_ddata_t *data, io_ddata_t *outputData, T lo, T hi)
{
  const size_t items = data->getItems();
  const size_t batches = std::min(data->getBatch(), outputData->getBatch());
  const size_t first = data->getBatch() - batches;

  // An output holding fewer batches gets the latest samples

  for (size_t b = 0; b < batches; b++)
    {
      for (size_t i = 0; i < items; i++)
        {
          outputData->get<T>(i, b) = std::min(std::max(data->get<T>(i, first + b), lo), hi);
        }
    }
}

void CProgSaturate::handle(CIOCommon *output,
                           io_ddata_t *data,
                           io_ddata_t *ioData,
                           io_ddata_t *outputData,
                           bool &initsample)
{
  const uint32_t lo = minRaw;
  const uint32_t hi = maxRaw;

  (void)ioData;
  (void)initsample;

  if (outputData->getDtype() != data->getDtype())
    {
      DAWNERR("saturate: output dtype %d differs from input dtype %d\n",
              outputData->getDtype(),
              data->getDtype());
      return;
    }

  switch (data->getDtype())
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        {
          clamp<uint8_t>(
            data, outputData, bound<uint8_t>(lo, hasMin, 0), bound<uint8_t>(hi, hasMax, 1));
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        {
          clamp<int8_t>(data,
                        outputData,
                        bound<int8_t>(lo, hasMin, INT8_MIN),
                        bound<int8_t>(hi, hasMax, INT8_MAX));
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        {
          clamp<uint8_t>(
            data, outputData, bound<uint8_t>(lo, hasMin, 0), bound<uint8_t>(hi, hasMax, UINT8_MAX));
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        {
          clamp<int16_t>(data,
                         outputData,
                         bound<int16_t>(lo, hasMin, INT16_MIN),
                         bound<int16_t>(hi, hasMax, INT16_MAX));
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        {
          clamp<uint16_t>(data,
                          outputData,
                          bound<uint16_t>(lo, hasMin, 0),
                          bound<uint16_t>(hi, hasMax, UINT16_MAX));
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        {
          clamp<int32_t>(data,
                         outputData,
                         bound<int32_t>(lo, hasMin, INT32_MIN),
                         bound<int32_t>(hi, hasMax, INT32_MAX));
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        {
          clamp<uint32_t>(data,
                          outputData,
                          bound<uint32_t>(lo, hasMin, 0),
                          bound<uint32_t>(hi, hasMax, UINT32_MAX));
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
        {
          // 16.16 fixed point is clamped as its raw int32 word

          clamp<int32_t>(data,
                         outputData,
                         bound<int32_t>(lo, hasMin, INT32_MIN),
                         bound<int32_t>(hi, hasMax, INT32_MAX));
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        {
          clamp<float>(data,
                       outputData,
                       bound(lo, hasMin, std::numeric_limits<float>::lowest()),
                       bound(hi, hasMax, std::numeric_limits<float>::max()));
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case SObjectId::DTYPE_DOUBLE:
        {
          clamp<double>(data,
                        outputData,
                        bound(lo, hasMin, std::numeric_limits<double>::lowest()),
                        bound(hi, hasMax, std::numeric_limits<double>::max()));
          break;
        }
#endif

      default:
        {
          DAWNERR("saturate: unsupported dtype %d\n", data->getDtype());
          return;
        }
    }

  output->setData(*outputData);
}
