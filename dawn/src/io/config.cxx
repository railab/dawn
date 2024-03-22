// dawn/src/io/config.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/config.hxx"

#include <alloca.h>
#include <cstdint>
#include <cstring>
#include <vector>

using namespace dawn;

static size_t cfgDtypeSize(uint8_t dtype)
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

static size_t cfgWordsPerValue(uint8_t dtype)
{
  size_t dsize = cfgDtypeSize(dtype);

  if (dsize == sizeof(uint64_t))
    {
      return 0;
    }

  return 1;
}

static size_t cfgValueCount(uint32_t objcfg)
{
  size_t words = SObjectCfg::objectCfgGetSize(objcfg);
  size_t step = cfgWordsPerValue(SObjectCfg::objectCfgGetDtype(objcfg));

  if (step == 0 || words == 0 || words % step != 0)
    {
      return 0;
    }

  return words / step;
}

static int cfgWordsToTyped(uint8_t dtype,
                           const uint32_t *src,
                           size_t words,
                           void *dst,
                           size_t items)
{
  size_t i;
  size_t step = cfgWordsPerValue(dtype);

  if (step == 0 || src == nullptr || dst == nullptr || items * step != words)
    {
      return -EINVAL;
    }

  switch (dtype)
    {
#if defined(CONFIG_DAWN_DTYPE_BOOL) || defined(CONFIG_DAWN_DTYPE_UINT8) || \
  defined(CONFIG_DAWN_DTYPE_CHAR)
#  ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_CHAR
      case SObjectId::DTYPE_CHAR:
#  endif
        {
          uint8_t *ptr = reinterpret_cast<uint8_t *>(dst);

          for (i = 0; i < items; i++)
            {
              ptr[i] = static_cast<uint8_t>(src[i] & 0xff);
            }

          return OK;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        {
          int8_t *ptr = reinterpret_cast<int8_t *>(dst);

          for (i = 0; i < items; i++)
            {
              ptr[i] = static_cast<int8_t>(src[i] & 0xff);
            }

          return OK;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        {
          uint16_t *ptr = reinterpret_cast<uint16_t *>(dst);

          for (i = 0; i < items; i++)
            {
              ptr[i] = static_cast<uint16_t>(src[i] & 0xffff);
            }

          return OK;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        {
          int16_t *ptr = reinterpret_cast<int16_t *>(dst);

          for (i = 0; i < items; i++)
            {
              ptr[i] = static_cast<int16_t>(src[i] & 0xffff);
            }

          return OK;
        }
#endif

#if defined(CONFIG_DAWN_DTYPE_INT32) || defined(CONFIG_DAWN_DTYPE_UINT32) || \
  defined(CONFIG_DAWN_DTYPE_FLOAT) || defined(CONFIG_DAWN_DTYPE_B16) ||      \
  defined(CONFIG_DAWN_DTYPE_UB16) || defined(CONFIG_DAWN_DTYPE_BLOCK)
#  ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UB16
      case SObjectId::DTYPE_UB16:
#  endif
      case SObjectId::DTYPE_ANY:
#  ifdef CONFIG_DAWN_DTYPE_BLOCK
      case SObjectId::DTYPE_BLOCK:
#  endif
        {
          std::memcpy(dst, src, items * sizeof(uint32_t));
          return OK;
        }
#endif

      default:
        {
          return -ENOTSUP;
        }
    }
}

static int cfgTypedToWords(uint8_t dtype,
                           const void *src,
                           size_t items,
                           uint32_t *dst,
                           size_t words)
{
  size_t i;
  size_t step = cfgWordsPerValue(dtype);

  if (step == 0 || src == nullptr || dst == nullptr || items * step != words)
    {
      return -EINVAL;
    }

  switch (dtype)
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        {
          const uint8_t *ptr = reinterpret_cast<const uint8_t *>(src);

          for (i = 0; i < items; i++)
            {
              dst[i] = ptr[i] ? 1 : 0;
            }

          return OK;
        }
#endif

#if defined(CONFIG_DAWN_DTYPE_UINT8) || defined(CONFIG_DAWN_DTYPE_CHAR)
#  ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_CHAR
      case SObjectId::DTYPE_CHAR:
#  endif
        {
          const uint8_t *ptr = reinterpret_cast<const uint8_t *>(src);

          for (i = 0; i < items; i++)
            {
              dst[i] = ptr[i];
            }

          return OK;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        {
          const int8_t *ptr = reinterpret_cast<const int8_t *>(src);
          int32_t v;

          for (i = 0; i < items; i++)
            {
              v = ptr[i];
              std::memcpy(&dst[i], &v, sizeof(v));
            }

          return OK;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        {
          const uint16_t *ptr = reinterpret_cast<const uint16_t *>(src);

          for (i = 0; i < items; i++)
            {
              dst[i] = ptr[i];
            }

          return OK;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        {
          const int16_t *ptr = reinterpret_cast<const int16_t *>(src);
          int32_t v;

          for (i = 0; i < items; i++)
            {
              v = ptr[i];
              std::memcpy(&dst[i], &v, sizeof(v));
            }

          return OK;
        }
#endif

#if defined(CONFIG_DAWN_DTYPE_INT32) || defined(CONFIG_DAWN_DTYPE_UINT32) || \
  defined(CONFIG_DAWN_DTYPE_FLOAT) || defined(CONFIG_DAWN_DTYPE_B16) ||      \
  defined(CONFIG_DAWN_DTYPE_UB16) || defined(CONFIG_DAWN_DTYPE_BLOCK)
#  ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
#  endif
#  ifdef CONFIG_DAWN_DTYPE_UB16
      case SObjectId::DTYPE_UB16:
#  endif
      case SObjectId::DTYPE_ANY:
#  ifdef CONFIG_DAWN_DTYPE_BLOCK
      case SObjectId::DTYPE_BLOCK:
#  endif
        {
          std::memcpy(dst, src, items * sizeof(uint32_t));
          return OK;
        }
#endif

      default:
        {
          return -ENOTSUP;
        }
    }
}

void CIOConfig::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls == CIOCommon::IO_CLASS_ANY)
        {
          offset += cfgCmnOffset(item);
          continue;
        }

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_CONFIG)
        {
          DAWNERR("unsupported config cfg 0x%08" PRIx32 "\n", item->cfgid.v);
          objcfg = 0;
          return;
        }

      switch (item->cfgid.s.id)
        {
          case CIOConfig::IO_CONFIG_CFG_OBJCFG:
            {
              const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);

              objcfg = *tmp;
              offset += 2;
              break;
            }

          case CIOConfig::IO_CONFIG_CFG_OFFSET:
            {
              const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);

              cfg_offset = *tmp;
              offset += 2;
              break;
            }

          case CIOConfig::IO_CONFIG_CFG_SIZE:
            {
              const uint32_t *tmp = reinterpret_cast<const uint32_t *>(&item->data);

              cfg_size = *tmp;
              offset += 2;
              break;
            }

          case CIOConfig::IO_CONFIG_CFG_ALLOC:
            {
              offset += 1;

              for (size_t j = 0; j < item->cfgid.s.size; j++)
                {
                  map.insert_or_assign(item->data[j], nullptr);

                  offset += 1;
                }

              break;
            }

          default:
            {
              DAWNERR("unsupported config cfg 0x%08" PRIx32 "\n", item->cfgid.v);
              objcfg = 0;
              return;
            }
        }
    }

  return;
}

CIOConfig::~CIOConfig()
{
}

int CIOConfig::configure()
{
  size_t cfg_words;
  uint8_t cfg_dtype;
  const CIOLimits *lim;
  bool has_limits;

  objcfg = 0;
  cfg_offset = 0;
  cfg_size = 0;

  // Configure

  configureDesc(getDesc());

  if (objcfg == 0)
    {
      return -EINVAL;
    }

  cfg_words = SObjectCfg::objectCfgGetSize(objcfg);

  if (cfg_size > 0 && cfg_offset + cfg_size > cfg_words)
    {
      DAWNERR("config: offset %" PRIu32 " + size %" PRIu32 " exceeds field size %zu\n",
              cfg_offset,
              cfg_size,
              cfg_words);
      return -EINVAL;
    }

  lim = &getCmnLimits();
  has_limits = lim->isConfigured();

  if (!has_limits)
    {
      return OK;
    }

  if (lim->getMin() == nullptr || lim->getMax() == nullptr || lim->getStep() == nullptr)
    {
      return -EINVAL;
    }

  cfg_words = cfg_size > 0 ? cfg_size : cfg_words;
  cfg_dtype = SObjectCfg::objectCfgGetDtype(objcfg);
  if (cfg_words == 0 ||
      (lim->getWords() != cfg_words && lim->getWords() != cfgWordsPerValue(cfg_dtype)))
    {
      return -EINVAL;
    }
  if (cfg_dtype != getDtype())
    {
      return -EINVAL;
    }

  return OK;
}

int CIOConfig::deinit()
{
  return OK;
}

int CIOConfig::getDataImpl(IODataCmn &data, size_t len)
{
  uint8_t dtype;
  size_t cfg_words;
  size_t effective_words;
  size_t effective_items;
  uint32_t *cfg_tmp;
  const uint32_t *src;
  int ret;

  DAWNASSERT(objcfg != 0, "invalid data");
  UNUSED(len);

  dtype = SObjectCfg::objectCfgGetDtype(objcfg);
  cfg_words = SObjectCfg::objectCfgGetSize(objcfg);

  effective_words = cfg_size > 0 ? cfg_size : cfg_words;
  effective_items = effective_words;

  if (effective_items == 0 || data.getItems() < effective_items)
    {
      return -EINVAL;
    }

  if (cfg_size > 0 && cfg_offset + effective_words > cfg_words)
    {
      DAWNERR("config read out of bounds: offset %" PRIu32 " + %zu > %zu words\n",
              cfg_offset,
              effective_words,
              cfg_words);
      return -EINVAL;
    }

  cfg_tmp = reinterpret_cast<uint32_t *>(alloca(cfg_words * sizeof(uint32_t)));

  for (auto &[id, obj] : map)
    {
      UNUSED(id);

      if (obj == nullptr)
        {
          return -EACCES;
        }

      ret = obj->getObjConfig(objcfg, cfg_tmp, cfg_words);
      if (ret < 0)
        {
          return ret;
        }

      src = (cfg_size > 0) ? &cfg_tmp[cfg_offset] : cfg_tmp;

      ret = cfgWordsToTyped(dtype, src, effective_words, data.getDataPtr(), effective_items);
      return ret;
    }

  return -EACCES;
}

int CIOConfig::setDataImpl(IODataCmn &data)
{
  uint8_t dtype;
  size_t cfg_len;
  size_t write_words;
  size_t write_items;
  uint32_t *user_words;
  uint32_t *full;
  size_t idx;
  size_t applied;
  size_t rollback_idx;
  int rollback_ret;
  int ret;
  std::vector<uint32_t> backups;

  DAWNASSERT(objcfg != 0, "invalid data");

  dtype = SObjectCfg::objectCfgGetDtype(objcfg);
  cfg_len = SObjectCfg::objectCfgGetSize(objcfg);

  write_words = cfg_size > 0 ? cfg_size : cfg_len;
  write_items = write_words;

  if (data.getItems() < write_items)
    {
      return -EINVAL;
    }

  if (cfg_size > 0 && cfg_offset + write_words > cfg_len)
    {
      DAWNERR("config write out of bounds: offset %" PRIu32 " + %zu > %zu words\n",
              cfg_offset,
              write_words,
              cfg_len);
      return -EINVAL;
    }

  // Convert user typed data to words (only the sub-range)
  user_words = reinterpret_cast<uint32_t *>(alloca(write_words * sizeof(uint32_t)));
  ret = cfgTypedToWords(dtype, data.getDataPtr(), write_items, user_words, write_words);
  if (ret < 0)
    {
      return ret;
    }

  ret = getCmnLimits().validate(user_words, write_words, dtype);
  if (ret < 0)
    {
      return ret;
    }

  if (cfg_len == 0)
    {
      return -EINVAL;
    }

  if (map.empty())
    {
      return -EACCES;
    }

  for (auto &[id, obj] : map)
    {
      UNUSED(id);

      if (obj == nullptr)
        {
          return -EACCES;
        }
    }

  // Build the full config data to write
  full = reinterpret_cast<uint32_t *>(alloca(cfg_len * sizeof(uint32_t)));

  if (cfg_size > 0)
    {
      // Read-RMW: read full field, patch sub-range, write back
      ret = map.begin()->second->getObjConfig(objcfg, full, cfg_len);
      if (ret < 0)
        {
          return ret;
        }

      std::memcpy(&full[cfg_offset], user_words, write_words * sizeof(uint32_t));
    }
  else
    {
      std::memcpy(full, user_words, cfg_len * sizeof(uint32_t));
    }

  // Backup current state for all targets
  backups.resize(map.size() * cfg_len);
  idx = 0;
  for (auto &[id, obj] : map)
    {
      UNUSED(id);
      ret = obj->getObjConfig(objcfg, &backups[idx * cfg_len], cfg_len);
      if (ret < 0)
        {
          return ret;
        }

      idx++;
    }

  // Apply to all targets with rollback
  applied = 0;
  idx = 0;
  for (auto &[id, obj] : map)
    {
      UNUSED(id);

      ret = obj->setObjConfig(objcfg, full, cfg_len);
      if (ret < 0)
        {
          rollback_idx = 0;
          for (auto &[rid, robj] : map)
            {
              UNUSED(rid);
              if (rollback_idx >= applied)
                {
                  break;
                }

              rollback_ret = robj->setObjConfig(objcfg, &backups[rollback_idx * cfg_len], cfg_len);
              if (rollback_ret < 0)
                {
                  DAWNERR("config rollback failed: %d\n", rollback_ret);
                }

              rollback_idx++;
            }

          return ret;
        }

      idx++;
      applied = idx;
    }

  return OK;
}

size_t CIOConfig::getDataSize() const
{
  uint8_t dtype;
  size_t dsize;
  size_t items;

  if (objcfg == 0)
    {
      return sizeof(uint32_t);
    }

  dtype = SObjectCfg::objectCfgGetDtype(objcfg);
  dsize = cfgDtypeSize(dtype);
  items = cfg_size > 0 ? cfg_size : cfgValueCount(objcfg);
  if (items == 0)
    {
      return sizeof(uint32_t);
    }

  return dsize * items;
}

size_t CIOConfig::getDataDim() const
{
  if (objcfg == 0)
    {
      return 1;
    }

  return cfg_size > 0 ? cfg_size : cfgValueCount(objcfg);
}

int CIOConfig::bind(CObject *obj, uint32_t id)
{
  DAWNASSERT(obj->getIdV() == id, "something goes wrong!");
  map.insert_or_assign(id, obj);

  return OK;
}
