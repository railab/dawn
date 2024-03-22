// dawn/src/prog/toggle.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/toggle.hxx"

#include <cstring>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

CProgToggle::CProgToggle(CDescObject &desc)
  : CProgCommon(desc)
  , onVal(1)
  , offVal(0)
  , curVal(0)
  , active(false)
  , registered(false)
{
}

CProgToggle::~CProgToggle()
{
  deinit();
}

int CProgToggle::allocBind(SObjectId::ObjectId sourceId, SObjectId::ObjectId outputId)
{
  SToggleBind *bind;

  bind = new (std::nothrow) SToggleBind();
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

int CProgToggle::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  const SObjectCfg::ObjectCfgData_t *raw;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_TOGGLE)
        {
          DAWNERR("toggle: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_TOGGLE_CFG_IOBIND:
            {
              size_t nitems = static_cast<size_t>(item->cfgid.s.size);
              if (nitems == 0 || nitems % 2 != 0)
                {
                  DAWNERR("toggle: invalid IOBIND size %zu\n", nitems);
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

          case PROG_TOGGLE_CFG_VALUES:
            {
              if (item->cfgid.s.size != 2)
                {
                  DAWNERR("toggle: VALUES config must have exactly 2 entries\n");
                  return -EINVAL;
                }

              raw = reinterpret_cast<const SObjectCfg::ObjectCfgData_t *>(item->data);
              offVal = SObjectCfg::cfgToU32(raw[0]);
              onVal = SObjectCfg::cfgToU32(raw[1]);
              break;
            }

          default:
            {
              DAWNERR("toggle: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (binds.empty())
    {
      DAWNERR("toggle: at least one IOBIND entry required\n");
      return -EINVAL;
    }

  curVal = offVal;
  return OK;
}

int CProgToggle::configure()
{
  return configureDesc(getDesc());
}

int CProgToggle::init()
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
          DAWNERR("toggle: source 0x%" PRIx32 " is not readable\n", bind->sourceId);
          return -EINVAL;
        }

      ret = prepareWritableTarget(bind->output, 1, true);
      if (ret != OK)
        {
          return ret;
        }

      if (bind->output->getDtype() != SObjectId::DTYPE_UINT32 || bind->output->getDataDim() != 1)
        {
          DAWNERR("toggle: output 0x%" PRIx32 " must be writable scalar uint32\n", bind->outputId);
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

int CProgToggle::deinit()
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

uint32_t CProgToggle::readInput(SToggleBind *bind, io_ddata_t *data)
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

void CProgToggle::writeValue(SToggleBind *bind)
{
  bind->outputData(0) = curVal;
  int ret = bind->output->setData(bind->outputData);
  if (ret != OK)
    {
      DAWNERR("toggle: setData failed %d\n", ret);
    }
}

void CProgToggle::refresh(SToggleBind *bind, io_ddata_t *data)
{
  uint32_t input = readInput(bind, data);

  if (bind->prevInput == 0 && input != 0)
    {
      curVal = curVal == offVal ? onVal : offVal;
      writeValue(bind);
    }

  bind->prevInput = input;
}

int CProgToggle::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SToggleBind *bind = static_cast<SToggleBind *>(priv);

  if (bind && bind->owner && bind->owner->active)
    {
      bind->owner->refresh(bind, data);
    }

  return OK;
}

int CProgToggle::doStart()
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
      writeValue(bind);
    }

  return OK;
}

int CProgToggle::doStop()
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

bool CProgToggle::hasThread() const
{
  return false;
}

int CProgToggle::trigger(uint8_t cmd)
{
  if (cmd == CMD_RESET)
    {
      curVal = offVal;
      for (auto bind : binds)
        {
          bind->prevInput = 0;
        }
      return OK;
    }

  return -ENOTSUP;
}
