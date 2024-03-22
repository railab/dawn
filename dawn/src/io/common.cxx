// dawn/src/io/common.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/common.hxx"

#include <climits>
#include <new>

using namespace dawn;

int CIOCommon::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemAtOffset(offset);

      if (item->cfgid.s.cls != CIOCommon::IO_CLASS_ANY)
        {
          offset += item->cfgid.s.size + 1;
          continue;
        }

      switch (item->cfgid.s.id)
        {
          case CIOCommon::IO_CFG_DEVNO:
            {
              if (item->data[0] > INT_MAX)
                {
                  return -ERANGE;
                }

              devno = static_cast<int>(item->data[0]);
              offset += 2;
              break;
            }

          case CIOCommon::IO_CFG_LIMIT_MIN:
            {
              if (limits.bind(CIOLimits::CFG_LIMIT_MIN,
                              item->cfgid.s.dtype,
                              item->cfgid.s.size,
                              reinterpret_cast<const uint32_t *>(&item->data[0])) < 0)
                {
                  return -EINVAL;
                }

              offset += item->cfgid.s.size + 1;
              break;
            }

          case CIOCommon::IO_CFG_LIMIT_MAX:
            {
              if (limits.bind(CIOLimits::CFG_LIMIT_MAX,
                              item->cfgid.s.dtype,
                              item->cfgid.s.size,
                              reinterpret_cast<const uint32_t *>(&item->data[0])) < 0)
                {
                  return -EINVAL;
                }

              offset += item->cfgid.s.size + 1;
              break;
            }

          case CIOCommon::IO_CFG_LIMIT_STEP:
            {
              if (limits.bind(CIOLimits::CFG_LIMIT_STEP,
                              item->cfgid.s.dtype,
                              item->cfgid.s.size,
                              reinterpret_cast<const uint32_t *>(&item->data[0])) < 0)
                {
                  return -EINVAL;
                }

              offset += item->cfgid.s.size + 1;
              break;
            }

          case CIOCommon::IO_CFG_NOTIFY:
            {
#ifdef CONFIG_DAWN_IO_NOTIFY
              notifyType = static_cast<uint8_t>(item->data[0]);
              notifyPrio = static_cast<int>(item->data[1]);
              notifyBatch = static_cast<size_t>(item->data[2]);
              if (notifyBatch == 0)
                {
                  notifyBatch = 1;
                }
#endif
              offset += 4;
              break;
            }

          default:
            {
              DAWNERR("Unsupported common config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

CIOCommon::CIOCommon(CDescObject &desc)
  : CObject(desc)
{
  DAWNASSERT(desc.getObjectId().s.cls > IO_CLASS_ANY && desc.getObjectId().s.cls < IO_CLASS_LAST,
             "invalid class");

  // Reset state

#ifdef CONFIG_DAWN_IO_NOTIFY
  notifier = nullptr;
  notifyType = IO_NOTIFY_POLL;
  notifyPrio = 0;
  notifyBatch = 1;
#endif

  limits.reset();

#ifdef CONFIG_DAWN_IO_HAS_STATS
  // Initialize statistics

  stats.read_count = 0;
  stats.write_count = 0;
  stats.error_count = 0;
#endif

  // By default devno is equal to instance number.

  devno = getPriv();

  // Configure common IO data

  configureDesc(desc);
}

size_t CIOCommon::cfgCmnOffset(const SObjectCfg::SObjectCfgItem *cfg)
{
  switch (cfg->cfgid.s.cls)
    {
      case CIOCommon::IO_CLASS_ANY:
        {
          switch (cfg->cfgid.s.id)
            {
              case CIOCommon::IO_CFG_DEVNO:
                {
                  return 2;
                }

              case CIOCommon::IO_CFG_LIMIT_MIN:
              case CIOCommon::IO_CFG_LIMIT_MAX:
              case CIOCommon::IO_CFG_LIMIT_STEP:
                {
                  return cfg->cfgid.s.size + 1;
                }

              case CIOCommon::IO_CFG_NOTIFY:
                {
                  return 4;
                }

              default:
                {
                  DAWNERR("unsupported common cfg 0x08%" PRIx32 "\n", cfg->cfgid.v);
                  return -1;
                }
            }

          break;
        }

      default:
        {
          DAWNERR("unsupported common cfg 0x08%" PRIx32 "\n", cfg->cfgid.v);
          return -1;
        }
    }

  return 0;
}

bool CIOCommon::isTimestamp() const
{
  return getFlags() & IO_FLAGS_TS;
}

uint64_t CIOCommon::getTimestamp()
{
#if defined(CONFIG_DAWN_IO_TIMESTAMP) || defined(CONFIG_DAWN_IO_TIMESTAMPIO)
  struct timespec ts;

  clock_systime_timespec(&ts);
  return 1000000ull * ts.tv_sec + ts.tv_nsec / 1000;
#else
  return 0;
#endif
}

#ifdef CONFIG_DAWN_IO_NOTIFY
void CIOCommon::bindNotifier(IIONotifier *n)
{
  DAWNASSERT(n, "notifier is nullptr");

  notifier = n;
}

int CIOCommon::setNotifier(IIONotifier::notifier_cb_t cb, int prio, void *priv)
{
  IIONotifier::SIONotifier n;

  // Notifier not supported

  if (!notifier)
    {
      DAWNERR("notifier=NULL!\n");
      return -EPERM;
    }

  // Fill notifier data

  n.priv = priv;
  n.io = this;
  n.cb = cb;
  n.prio = prio;

  return notifier->regNotifier(n);
}

int CIOCommon::notifyData(io_ddata_t *data)
{
  if (!notifier)
    {
      return OK;
    }

  return notifier->notifyData(this, data);
}
#endif

io_ddata_t *CIOCommon::ddata_alloc(size_t batch, size_t chunk_size)
{
  size_t dim = getDataDim();
  size_t dsize = getDataSize();
  size_t tsize;
#ifdef CONFIG_DAWN_IO_SEEKABLE
  size_t chunk;

  // Seekable IOs use a chunk buffer (T=1, UINT8) capped to chunk_size.

  if (isSeekable())
    {
      if (chunk_size == 0)
        {
          DAWNERR("ddata_alloc: seekable IO requires chunk_size > 0\n");
          return nullptr;
        }

      chunk = dsize > 0 && chunk_size < dsize ? chunk_size : dsize;
      if (chunk == 0)
        {
          chunk = chunk_size;
        }

      io_ddata_t *data = new (std::nothrow) io_ddata_t(1, chunk, batch, SObjectId::DTYPE_UINT8);
      if (data == nullptr || !data->isAllocated())
        {
          delete data;
          return nullptr;
        }

      return data;
    }
#endif

  if (dim == 0)
    {
      DAWNERR("dim == 0");
      return nullptr;
    }

  // check input

  tsize = dsize / dim;
  if (tsize == 0)
    {
      DAWNERR("tsize == 0");
      return nullptr;
    }

  io_ddata_t *data = new (std::nothrow) io_ddata_t(tsize, dim, batch, getDtype(), isTimestamp());
  if (data == nullptr || !data->isAllocated())
    {
      delete data;
      return nullptr;
    }

  return data;
}
