// dawn/src/prog/sampling.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/sampling.hxx"

#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

int CProgSampling::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item = nullptr;
  const SObjectId::UObjectId *ids;
  const uint32_t *data;
  size_t offset = 0;
  size_t nbind;
  size_t j;
  int ret;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_SAMPLING)
        {
          DAWNERR("Unsupported sampling config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_SAMPLING_CFG_IOBIND:
            {
              if (item->cfgid.s.size % 2 != 0)
                {
                  DAWNERR("Invalid IOBIND size %d, must be even\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              nbind = item->cfgid.s.size / 2;
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);

              for (j = 0; j < nbind; j++)
                {
                  ret = allocObject(ids[j * 2].v, ids[j * 2 + 1].v);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }

              break;
            }

          case PROG_SAMPLING_CFG_INTERVAL:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("Invalid INTERVAL size %d\n", item->cfgid.s.size);
                  return -EINVAL;
                }

              data = reinterpret_cast<const uint32_t *>(item->data);
              interval = *data;
              break;
            }

          default:
            {
              DAWNERR("Unsupported sampling config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CProgSampling::allocObject(SObjectId::ObjectId srcId, SObjectId::ObjectId targetId)
{
  SSamplingBind *b;

  DAWNINFO("allocate sampling 0x%" PRIx32 " -> 0x%" PRIx32 "\n", srcId, targetId);

  b = new (std::nothrow) SSamplingBind();
  if (!b)
    {
      return -ENOMEM;
    }

  b->srcId = srcId;
  b->targetId = targetId;
  b->src = nullptr;
  b->target = nullptr;
  b->iodata = nullptr;

  setObjectMapItem(b->srcId, nullptr);
  setObjectMapItem(b->targetId, nullptr);
  binds.push_back(b);

  return OK;
}

void CProgSampling::thread()
{
  SSamplingBind *b;
  size_t i;
  int ret;

  DAWNINFO("start sampling thread\n");

  do
    {
      // Sample all source IOs and push to targets

      for (i = 0; i < binds.size(); i++)
        {
          b = binds[i];

          ret = b->src->getData(*b->iodata, 1);
          if (ret != OK)
            {
              DAWNERR("getData failed for src %zu (error %d)\n", i, ret);
              continue;
            }

          ret = b->target->setData(*b->iodata);
          if (ret != OK)
            {
              DAWNERR("setData failed for target %zu (error %d)\n", i, ret);
            }
        }

      // Wait for next interval

      usleep(interval);
    }
  while (!threadCtl.shouldQuit());
}

CProgSampling::~CProgSampling()
{
  deinit();
}

int CProgSampling::configure()
{
  int ret;

  // Configure object

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Sampling configure failed (error %d)\n", ret);
      return ret;
    }

  // One-time initialization that depends on bindings is done in init()

  return OK;
}

int CProgSampling::deinit()
{
  size_t i;

  // Free allocated data buffers

  for (i = 0; i < binds.size(); i++)
    {
      delete binds[i]->iodata;
      delete binds[i];
    }

  binds.clear();

  return OK;
}

int CProgSampling::init()
{
  SSamplingBind *b;
  size_t i;
  int ret;

  // Resolve IOs, initialize writable targets, allocate buffers

  for (i = 0; i < binds.size(); i++)
    {
      b = binds[i];

      b->src = getIO(b->srcId);
      if (!b->src)
        {
          DAWNERR("Source IO 0x%" PRIx32 " not found\n", b->srcId);
          return -EIO;
        }

      if (!b->src->isRead())
        {
          DAWNERR("Source IO 0x%" PRIx32 " is not readable\n", b->srcId);
          return -EINVAL;
        }

      b->target = getIO(b->targetId);
      if (!b->target)
        {
          DAWNERR("Target IO 0x%" PRIx32 " not found\n", b->targetId);
          return -EIO;
        }

      if (b->target->getDtype() != b->src->getDtype())
        {
          DAWNERR("Target IO 0x%" PRIx32 " dtype mismatch\n", b->targetId);
          return -EINVAL;
        }

      // Initialize deferred virtual targets and validate configured targets

      ret = prepareWritableTarget(b->target,
                                  b->src->getDataDim(),
#ifdef CONFIG_DAWN_IO_NOTIFY
                                  true
#else
                                  false
#endif
      );
      if (ret != OK)
        {
          DAWNERR("Failed to initialize target %zu (error %d)\n", i, ret);
          return ret;
        }

      if (b->target->getDataDim() != b->src->getDataDim() ||
          b->target->getDataSize() != b->src->getDataSize())
        {
          DAWNERR("Target IO 0x%" PRIx32 " shape mismatch\n", b->targetId);
          return -EINVAL;
        }

      // Allocate data buffer for this source

      b->iodata = b->src->ddata_alloc(1);
      if (!b->iodata)
        {
          DAWNERR("Failed to allocate data buffer for src %zu\n", i);
          return -ENOMEM;
        }
    }

  return OK;
}

int CProgSampling::doStart()
{
  // Start sampling thread

  threadCtl.setThreadFunc([this]() { thread(); });
  return threadCtl.threadStart();
}

int CProgSampling::doStop()
{
  return threadCtl.threadStop();
}

bool CProgSampling::hasThread() const
{
  return threadCtl.isRunning();
}
