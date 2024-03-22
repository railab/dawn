// dawn/src/prog/counter.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/counter.hxx"

#include <cstring>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

CProgCounter::CProgCounter(CDescObject &desc)
  : CProgCommon(desc)
  , countMin(0)
  , countMax(3)
  , countStep(1)
  , countInit(0)
  , count(0)
  , active(false)
  , registered(false)
{
}

CProgCounter::~CProgCounter()
{
  deinit();
}

int CProgCounter::allocBind(SObjectId::ObjectId sourceId, SObjectId::ObjectId outputId)
{
  SCounterBind *bind;

  bind = new (std::nothrow) SCounterBind();
  if (!bind)
    {
      return -ENOMEM;
    }

  bind->owner = this;
  bind->sourceId = sourceId;
  bind->outputId = outputId;
  bind->source = nullptr;
  bind->output = nullptr;
  bind->sourceData = nullptr;
  bind->prevInput = 0;

  binds.push_back(bind);
  setObjectMapItem(sourceId, nullptr);
  setObjectMapItem(outputId, nullptr);
  return OK;
}

int CProgCounter::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  const SObjectCfg::ObjectCfgData_t *raw;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_COUNTER)
        {
          DAWNERR("counter: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_COUNTER_CFG_IOBIND:
            {
              size_t nitems = static_cast<size_t>(item->cfgid.s.size);
              if (nitems == 0 || nitems % 2 != 0)
                {
                  DAWNERR("counter: invalid IOBIND size %zu\n", nitems);
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              for (size_t b = 0; b < nitems / 2; b++)
                {
                  int ret = allocBind(ids[b * 2].v, ids[b * 2 + 1].v);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }
              break;
            }

          case PROG_COUNTER_CFG_PARAMS:
            {
              if (item->cfgid.s.size != 4)
                {
                  DAWNERR("counter: PARAMS config must have exactly 4 entries\n");
                  return -EINVAL;
                }

              raw = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
              countMin = SObjectCfg::cfgToU32(raw[0]);
              countMax = SObjectCfg::cfgToU32(raw[1]);
              countStep = SObjectCfg::cfgToU32(raw[2]);
              countInit = SObjectCfg::cfgToU32(raw[3]);
              break;
            }

          default:
            {
              DAWNERR("counter: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (binds.empty())
    {
      DAWNERR("counter: at least one IOBIND entry required\n");
      return -EINVAL;
    }

  if (countStep == 0)
    {
      DAWNERR("counter: step must be > 0\n");
      return -EINVAL;
    }

  if (countMin > countMax)
    {
      DAWNERR("counter: min (%" PRIu32 ") > max (%" PRIu32 ")\n", countMin, countMax);
      return -EINVAL;
    }

  if (countInit < countMin || countInit > countMax)
    {
      DAWNERR("counter: initial (%" PRIu32 ") outside range [%" PRIu32 ", %" PRIu32 "]\n",
              countInit,
              countMin,
              countMax);
      return -EINVAL;
    }

  count = countInit;
  return OK;
}

int CProgCounter::configure()
{
  return configureDesc(getDesc());
}

int CProgCounter::init()
{
  int ret;

  for (auto bind : binds)
    {
      bind->source = getIO(bind->sourceId);
      bind->output = getIO(bind->outputId);
      if (!bind->source || !bind->output)
        {
          return -EIO;
        }

      if (!bind->source->isRead())
        {
          DAWNERR("counter: source 0x%" PRIx32 " is not readable\n", bind->sourceId);
          return -EINVAL;
        }

      ret = prepareWritableTarget(bind->output, 1, true);
      if (ret != OK)
        {
          return ret;
        }

      if (bind->output->getDtype() != SObjectId::DTYPE_UINT32 || bind->output->getDataDim() != 1)
        {
          DAWNERR("counter: output 0x%" PRIx32 " must be writable scalar uint32\n", bind->outputId);
          return -EINVAL;
        }

      bind->sourceData = bind->source->ddata_alloc(1);
      if (!bind->sourceData)
        {
          return -ENOMEM;
        }
    }

  return OK;
}

int CProgCounter::deinit()
{
  doStop();

  for (auto bind : binds)
    {
      delete bind->sourceData;
      delete bind;
    }

  binds.clear();
  return OK;
}

uint32_t CProgCounter::readInput(SCounterBind *bind, io_ddata_t *data)
{
  uint32_t input = 0;

  if (data && data->getItems() >= 1)
    {
      std::memcpy(
        bind->sourceData->getDataPtr(), data->getDataPtr(), bind->sourceData->getDataSize());
    }
  else if (bind->source->getData(*bind->sourceData, 1) != OK)
    {
      return 0;
    }

  std::memcpy(&input,
              bind->sourceData->getDataPtr(),
              bind->sourceData->getDataSize() < sizeof(input) ? bind->sourceData->getDataSize()
                                                              : sizeof(input));
  return input;
}

void CProgCounter::writeCount(SCounterBind *bind)
{
  bind->outputData(0) = count;
  int ret = bind->output->setData(bind->outputData);
  if (ret != OK)
    {
      DAWNERR("counter: setData failed %d\n", ret);
    }
}

void CProgCounter::refresh(SCounterBind *bind, io_ddata_t *data)
{
  uint32_t input = readInput(bind, data);

  if (bind->prevInput == 0 && input != 0)
    {
      uint32_t next = count + countStep;
      count = next > countMax ? countMin : next;
      writeCount(bind);
    }

  bind->prevInput = input;
}

int CProgCounter::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SCounterBind *bind = static_cast<SCounterBind *>(priv);

  if (bind && bind->owner && bind->owner->active)
    {
      bind->owner->refresh(bind, data);
    }

  return OK;
}

int CProgCounter::doStart()
{
  int ret;

  if (!registered)
    {
      for (auto bind : binds)
        {
          if (!bind->source->isNotify())
            {
              continue;
            }

          ret = bind->source->setNotifier(ioNotifierCb, 0, bind);
          if (ret != OK)
            {
              return ret;
            }
        }

      registered = true;
    }

  active = true;

  for (auto bind : binds)
    {
      bind->prevInput = readInput(bind, nullptr);
      writeCount(bind);
    }

  return OK;
}

int CProgCounter::doStop()
{
  if (registered)
    {
      for (auto bind : binds)
        {
          if (bind->source && bind->source->isNotify())
            {
              bind->source->setNotifier(nullptr, 0, nullptr);
            }
        }

      registered = false;
    }

  active = false;
  return OK;
}

bool CProgCounter::hasThread() const
{
  return false;
}

int CProgCounter::trigger(uint8_t cmd)
{
  if (cmd == CMD_RESET)
    {
      count = countInit;
      for (auto bind : binds)
        {
          bind->prevInput = 0;
        }
      return OK;
    }

  return -ENOTSUP;
}
