// dawn/src/prog/process.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/process.hxx"

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

int CProgProcess::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SProcessBind *bind = static_cast<SProcessBind *>(priv);

  if (!bind->active)
    {
      return OK;
    }

  // Get data and handle

  bind->owner->handleCmn(bind, data);
  return OK;
}

int CProgProcess::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      switch (item->cfgid.s.id)
        {
          case PROG_STATS_CFG_IOBIND:
            {
              const size_t wpe = sizeof(SProgStatsIOBind) / 4;
              size_t nbinds;
              size_t b;

              if (item->cfgid.s.size == 0 || item->cfgid.s.size % wpe != 0)
                {
                  DAWNERR(
                    "Invalid config size %d, expected multiple of %zu\n", item->cfgid.s.size, wpe);
                  return -EINVAL;
                }

              nbinds = item->cfgid.s.size / wpe;
              for (b = 0; b < nbinds; b++)
                {
                  SProgStatsIOBind *tmp =
                    reinterpret_cast<SProgStatsIOBind *>(item->data + b * wpe);

                  int ret = allocObject(tmp);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }

              break;
            }

          default:
            {
              int ret = configureExtraCfgItem(desc, item, offset);
              if (ret != OK)
                {
                  DAWNERR("Unsupported stats config 0x%08" PRIx32 "\n", item->cfgid.v);
                  return -EINVAL;
                }
              break;
            }
        }
    }

  return OK;
}

int CProgProcess::configureExtraCfgItem(const CDescObject &desc,
                                        const SObjectCfg::SObjectCfgItem *item,
                                        size_t &offset)
{
  (void)desc;
  (void)item;
  (void)offset;
  return -ENOTSUP;
}

int CProgProcess::bindStateAlloc(CIOCommon *src,
                                 CIOCommon *output,
                                 io_ddata_t *ioData,
                                 io_ddata_t *outputData,
                                 SBindState **state)
{
  (void)src;
  (void)output;
  (void)ioData;
  (void)outputData;

  *state = nullptr;
  return OK;
}

void CProgProcess::handleWithState(CIOCommon *output,
                                   io_ddata_t *data,
                                   io_ddata_t *ioData,
                                   io_ddata_t *outputData,
                                   bool &initsample,
                                   void *state)
{
  (void)state;
  handle(output, data, ioData, outputData, initsample);
}

int CProgProcess::allocObject(const SProgStatsIOBind *alloc)
{
  DAWNINFO("allocate prog 0x%" PRIx32 " -> 0x%" PRIx32 "\n", alloc->objid.v, alloc->output.v);

  // Allocate source and output in map

  setObjectMapItem(alloc->objid.v, nullptr);
  setObjectMapItem(alloc->output.v, nullptr);
  binds.push_back({this, *alloc});

  return OK;
};

int CProgProcess::bindPrepare(SProcessBind *bind)
{
  int ret;
  size_t dim;

  // Check IO property

  if (!bind->src->isNotify())
    {
      DAWNERR("Notifications not supported by IO 0x%" PRIx32 "\n", bind->src->getIdV());
      return -EINVAL;
    }

  if (!bind->src->isRead())
    {
      DAWNERR("Source IO 0x%" PRIx32 " is not readable\n", bind->src->getIdV());
      return -EINVAL;
    }

  // Register callback

  ret = bind->src->setNotifier(ioNotifierCb, 0, bind);
  if (ret < 0)
    {
      DAWNERR("Set notifier failed for objId = 0x%" PRIx32 "\n", bind->src->getIdV());
      return ret;
    }

  // Get dimension

  dim = bind->src->getDataDim();

  // Initialize deferred virtual outputs and validate configured targets.

  ret = prepareWritableTarget(bind->output, dim, true);
  if (ret != OK)
    {
      DAWNERR("Failed to initialize output IO (error %d)\n", ret);
      return ret;
    }

  if (bind->output->getDataDim() != dim)
    {
      DAWNERR("Process output 0x%" PRIx32 " shape mismatch\n", bind->output->getIdV());
      return -EINVAL;
    }

  return OK;
}

CProgProcess::~CProgProcess()
{
  deinit();
}

int CProgProcess::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Stats configure failed (error %d)\n", ret);
      return ret;
    }

  // One-time initialization that depends on bindings is done in init()

  return OK;
}

int CProgProcess::init()
{
  int ret;

  for (auto &bind : binds)
    {
      bind.src = getIO(bind.cfg.objid.v);
      if (!bind.src)
        {
          return -EIO;
        }

      bind.output = getIO(bind.cfg.output.v);
      if (!bind.output)
        {
          return -EIO;
        }

      ret = bindPrepare(&bind);
      if (ret != OK)
        {
          return ret;
        }

      bind.ioData = bind.src->ddata_alloc(1);
      if (!bind.ioData)
        {
          return -ENOMEM;
        }

      bind.outputData = bind.output->ddata_alloc(1);
      if (!bind.outputData)
        {
          delete bind.ioData;
          bind.ioData = nullptr;
          return -ENOMEM;
        }

      ret = bindStateAlloc(bind.src, bind.output, bind.ioData, bind.outputData, &bind.state);
      if (ret != OK)
        {
          delete bind.ioData;
          bind.ioData = nullptr;
          delete bind.outputData;
          bind.outputData = nullptr;
          return ret;
        }
    }

  return OK;
}

int CProgProcess::deinit()
{
  for (auto &bind : binds)
    {
      delete bind.state;
      bind.state = nullptr;

      delete bind.ioData;

      bind.ioData = nullptr;

      delete bind.outputData;

      bind.outputData = nullptr;
      bind.src = nullptr;
      bind.output = nullptr;
    }

  binds.clear();

  return OK;
}

int CProgProcess::doStart()
{
  // Mark first sample as init sample

  for (auto &bind : binds)
    {
      bind.initsample = true;
      bind.active = true;
    }

  return OK;
};

int CProgProcess::doStop()
{
  for (auto &bind : binds)
    {
      bind.active = false;
    }

  return OK;
};

bool CProgProcess::hasThread() const
{
  return false;
}

int CProgProcess::trigger(uint8_t cmd)
{
  if (cmd == CMD_RESET)
    {
      for (auto &bind : binds)
        {
          bind.initsample = true;
          if (bind.state != nullptr)
            {
              bind.state->reset();
            }
        }

      return OK;
    }

  return -ENOTSUP;
}
