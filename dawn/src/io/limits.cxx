// dawn/src/io/limits.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/limits.hxx"

#include <cmath>
#include <cstdint>
#include <cstring>

using namespace dawn;

static size_t limitsDtypeSize(uint8_t dtype)
{
  int sz;

  if (dtype == SObjectId::DTYPE_ANY)
    {
      return sizeof(uint32_t);
    }

  sz = SObjectId::getDtypeSize_((SObjectId::EObjectDataType)dtype);
  if (sz <= 0)
    {
      return sizeof(uint32_t);
    }

  return (size_t)sz;
}

static size_t limitsWordsPerValue(uint8_t dtype)
{
  size_t dsize = limitsDtypeSize(dtype);
  (void)dsize;

  return 1;
}

int CIOLimits::bind(uint8_t id, uint8_t dtype, size_t words, const uint32_t *data)
{
  if (data == nullptr || words == 0)
    {
      return -EINVAL;
    }

  if (nWords == 0)
    {
      nWords = words;
      cfgDtype = dtype;
    }
  else
    {
      if (nWords != words || cfgDtype != dtype)
        {
          return -EINVAL;
        }
    }

  switch (id)
    {
      case CFG_LIMIT_MIN:
        {
          minData = data;
          return OK;
        }
      case CFG_LIMIT_MAX:
        {
          maxData = data;
          return OK;
        }
      case CFG_LIMIT_STEP:
        {
          stepData = data;
          return OK;
        }
      default:
        {
          return -EINVAL;
        }
    }
}

int CIOLimits::validate(const uint32_t *data, size_t words, uint8_t dtype) const
{
  size_t items;
  size_t step_words;
  size_t i;

  if (!isConfigured())
    {
      return OK;
    }

  if (data == nullptr || minData == nullptr || maxData == nullptr || stepData == nullptr)
    {
      return -EINVAL;
    }

  if (dtype != cfgDtype)
    {
      return -EINVAL;
    }

  if (words != nWords)
    {
      return -EINVAL;
    }

  step_words = limitsWordsPerValue(dtype);
  if (step_words == 0 || words % step_words != 0)
    {
      return -EINVAL;
    }

  items = words / step_words;

  for (i = 0; i < items; i++)
    {
      switch (dtype)
        {
#if defined(CONFIG_DAWN_DTYPE_BOOL) || defined(CONFIG_DAWN_DTYPE_UINT8) ||  \
  defined(CONFIG_DAWN_DTYPE_UINT16) || defined(CONFIG_DAWN_DTYPE_UINT32) || \
  defined(CONFIG_DAWN_DTYPE_CHAR)
#  ifdef CONFIG_DAWN_DTYPE_BOOL
          case SObjectId::DTYPE_BOOL:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
          case SObjectId::DTYPE_UINT8:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT16
          case SObjectId::DTYPE_UINT16:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
          case SObjectId::DTYPE_UINT32:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_CHAR
          case SObjectId::DTYPE_CHAR:
#  endif
            {
              uint32_t v;
              uint32_t min;
              uint32_t max;
              uint32_t step;
              size_t off = i * step_words;

              v = data[i];
              min = minData[off];
              max = maxData[off];
              step = stepData[off];

              if (v < min || v > max)
                {
                  return -ERANGE;
                }

              if (step > 0 && ((v - min) % step) != 0)
                {
                  return -ERANGE;
                }

              break;
            }
#endif

#if defined(CONFIG_DAWN_DTYPE_INT8) || defined(CONFIG_DAWN_DTYPE_INT16) || \
  defined(CONFIG_DAWN_DTYPE_INT32)
#  ifdef CONFIG_DAWN_DTYPE_INT8
          case SObjectId::DTYPE_INT8:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT16
          case SObjectId::DTYPE_INT16:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_INT32
          case SObjectId::DTYPE_INT32:
#  endif
            {
              int32_t v;
              int32_t min;
              int32_t max;
              int32_t step;
              int32_t min32;
              int32_t max32;
              int32_t step32;
              size_t off = i * step_words;

              std::memcpy(&v, &data[i], sizeof(v));

              std::memcpy(&min32, &minData[off], sizeof(min32));
              std::memcpy(&max32, &maxData[off], sizeof(max32));
              std::memcpy(&step32, &stepData[off], sizeof(step32));
              min = min32;
              max = max32;
              step = step32;

              if (v < min || v > max)
                {
                  return -ERANGE;
                }

              if (step > 0 && ((v - min) % step) != 0)
                {
                  return -ERANGE;
                }

              break;
            }
#endif

#if defined(CONFIG_DAWN_DTYPE_B16) || defined(CONFIG_DAWN_DTYPE_UB16)
#  ifdef CONFIG_DAWN_DTYPE_B16
          case SObjectId::DTYPE_B16:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UB16
          case SObjectId::DTYPE_UB16:
#  endif
            {
              int32_t v;
              int32_t min;
              int32_t max;
              int32_t step;
              size_t off = i * step_words;

              std::memcpy(&v, &data[i], sizeof(v));
              std::memcpy(&min, &minData[off], sizeof(min));
              std::memcpy(&max, &maxData[off], sizeof(max));
              std::memcpy(&step, &stepData[off], sizeof(step));

              if (v < min || v > max)
                {
                  return -ERANGE;
                }

              if (step > 0 && ((v - min) % step) != 0)
                {
                  return -ERANGE;
                }

              break;
            }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
          case SObjectId::DTYPE_FLOAT:
            {
              float v;
              float min;
              float max;
              float step;
              float rem;
              float eps;
              size_t off = i * step_words;

              v = SObjectCfg::cfgToF(data[i]);
              min = SObjectCfg::cfgToF(minData[off]);
              max = SObjectCfg::cfgToF(maxData[off]);
              step = SObjectCfg::cfgToF(stepData[off]);

              if (v < min || v > max)
                {
                  return -ERANGE;
                }

              if (step > 0.0f)
                {
                  rem = std::fmod(v - min, step);
                  eps = step * 1e-6f;

                  if (!(std::fabs(rem) <= eps || std::fabs(rem - step) <= eps))
                    {
                      return -ERANGE;
                    }
                }

              break;
            }
#endif

          default:
            {
              return -ENOTSUP;
            }
        }
    }

  return OK;
}
