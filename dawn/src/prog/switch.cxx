// dawn/src/prog/switch.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/switch.hxx"

#include <cstring>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"
#include "dawn/io/sdata.hxx"

using namespace dawn;

static const size_t INPUT_WORDS = 2;

CProgSwitch::CProgSwitch(CDescObject &desc)
  : CProgCommon(desc)
  , target(nullptr)
  , targetId(0)
  , iodata(nullptr)
  , onCmd(1)
  , offCmd(0)
  , lastAndState(false)
  , active(false)
  , registered(false)
{
}

CProgSwitch::~CProgSwitch()
{
  deinit();
}

int CProgSwitch::allocInput(SObjectId::ObjectId ioId, uint32_t match)
{
  SSwitchInput inp;

  inp.owner = this;
  inp.io = nullptr;
  inp.ioId = ioId;
  inp.match = match;
  inp.current = 0;

  inputs.push_back(inp);
  setObjectMapItem(ioId, nullptr);
  return OK;
}

int CProgSwitch::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  const uint32_t *vals;
  size_t offset;
  size_t ii;
  size_t n;

  offset = 0;
  for (ii = 0; ii < desc.getSize(); ii++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_SWITCH)
        {
          DAWNERR("switch: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_SWITCH_CFG_INPUTS:
            {
              n = static_cast<size_t>(item->cfgid.s.size);
              if (n == 0 || n % INPUT_WORDS != 0)
                {
                  DAWNERR("switch: invalid INPUTS size %zu\n", n);
                  return -EINVAL;
                }

              vals = reinterpret_cast<const uint32_t *>(item->data);
              for (size_t j = 0; j < n; j += INPUT_WORDS)
                {
                  ids = reinterpret_cast<const SObjectId::UObjectId *>(&vals[j]);
                  int ret = allocInput(ids[0].v, vals[j + 1]);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }
              break;
            }

          case PROG_SWITCH_CFG_TARGET:
            {
              n = static_cast<size_t>(item->cfgid.s.size);
              if (n != 3)
                {
                  DAWNERR("switch: invalid TARGET size %zu\n", n);
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              vals = reinterpret_cast<const uint32_t *>(item->data);
              targetId = ids[0].v;
              onCmd = static_cast<uint8_t>(vals[1]);
              offCmd = static_cast<uint8_t>(vals[2]);
              setObjectMapItem(targetId, nullptr);
              break;
            }

          default:
            {
              DAWNERR("switch: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (inputs.empty())
    {
      DAWNERR("switch: at least one input required\n");
      return -EINVAL;
    }

  if (targetId == 0)
    {
      DAWNERR("switch: target IO not configured\n");
      return -EINVAL;
    }

  return OK;
}

int CProgSwitch::configure()
{
  return configureDesc(getDesc());
}

int CProgSwitch::init()
{
  size_t i;
  CIOCommon *io;
  int ret;

  for (i = 0; i < inputs.size(); i++)
    {
      io = getIO(inputs[i].ioId);
      if (!io)
        {
          DAWNERR("switch: input IO 0x%" PRIx32 " not found\n", inputs[i].ioId);
          return -EIO;
        }

      if (!io->isRead())
        {
          DAWNERR("switch: input 0x%" PRIx32 " is not readable\n", inputs[i].ioId);
          return -EINVAL;
        }

      if (io->getDtype() != SObjectId::DTYPE_UINT32 || io->getDataDim() != 1)
        {
          DAWNERR("switch: input 0x%" PRIx32 " must be scalar uint32, got dtype=%u dim=%zu\n",
                  inputs[i].ioId,
                  io->getDtype(),
                  io->getDataDim());
          return -EINVAL;
        }

      inputs[i].io = io;
    }

  target = getIO(targetId);
  if (!target)
    {
      DAWNERR("switch: target 0x%" PRIx32 " not found\n", targetId);
      return -EIO;
    }

  ret = prepareWritableTarget(target, 1, true);
  if (ret != OK)
    {
      DAWNERR("switch: target prepare failed %d\n", ret);
      return ret;
    }

  iodata = new (std::nothrow) io_ddata_t(target->getDataSize(), 1, 1, target->getDtype());
  if (!iodata || !iodata->isAllocated())
    {
      delete iodata;
      iodata = nullptr;
      DAWNERR("switch: iodata allocation failed\n");
      return -ENOMEM;
    }

  return OK;
}

int CProgSwitch::deinit()
{
  doStop();
  delete iodata;
  iodata = nullptr;
  target = nullptr;
  inputs.clear();
  return OK;
}

int CProgSwitch::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SSwitchInput *inp = static_cast<SSwitchInput *>(priv);

  if (data && data->getItems() >= 1)
    {
      std::memcpy(&inp->current, data->getDataPtr(), sizeof(inp->current));
    }

  if (inp->owner && inp->owner->active)
    {
      inp->owner->evaluate();
    }

  return OK;
}

int CProgSwitch::doStart()
{
  size_t i;
  int ret;

  if (registered)
    {
      active = true;

      for (i = 0; i < inputs.size(); i++)
        {
          io_sdata_t<uint32_t, 1, 1> inputData;

          ret = inputs[i].io->getData(inputData, 1);
          if (ret != OK)
            {
              DAWNERR("switch: getData failed on input %zu: %d\n", i, ret);
              return ret;
            }
          inputs[i].current = inputData(0);
        }

      lastAndState = !allInputsMatch();
      evaluate();
      return OK;
    }

  active = true;

  for (i = 0; i < inputs.size(); i++)
    {
      if (!inputs[i].io->isNotify())
        {
          continue;
        }

      ret = inputs[i].io->setNotifier(ioNotifierCb, 0, &inputs[i]);
      if (ret != OK)
        {
          DAWNERR("switch: setNotifier failed on input %zu: %d\n", i, ret);
          return ret;
        }
    }

  registered = true;

  // Read current values and force the first write unconditionally.
  for (i = 0; i < inputs.size(); i++)
    {
      io_sdata_t<uint32_t, 1, 1> inputData;

      ret = inputs[i].io->getData(inputData, 1);
      if (ret != OK)
        {
          DAWNERR("switch: getData failed on input %zu: %d\n", i, ret);
          return ret;
        }
      inputs[i].current = inputData(0);
    }
  lastAndState = !allInputsMatch();
  evaluate();

  return OK;
}

int CProgSwitch::doStop()
{
  if (registered)
    {
      for (size_t i = 0; i < inputs.size(); i++)
        {
          if (inputs[i].io && inputs[i].io->isNotify())
            {
              inputs[i].io->setNotifier(nullptr, 0, nullptr);
            }
        }

      registered = false;
    }

  active = false;
  return OK;
}

bool CProgSwitch::hasThread() const
{
  return false;
}

bool CProgSwitch::allInputsMatch() const
{
  for (size_t i = 0; i < inputs.size(); i++)
    {
      if (inputs[i].current != inputs[i].match)
        {
          return false;
        }
    }

  return true;
}

void CProgSwitch::evaluate()
{
  bool allMatch;
  uint8_t cmd;
  int ret;

  for (size_t i = 0; i < inputs.size(); i++)
    {
      io_sdata_t<uint32_t, 1, 1> inputData;

      if (inputs[i].io == nullptr)
        {
          return;
        }

      ret = inputs[i].io->getData(inputData, 1);
      if (ret != OK)
        {
          DAWNERR("switch: getData failed on input %zu: %d\n", i, ret);
          return;
        }

      inputs[i].current = inputData(0);
    }

  allMatch = allInputsMatch();

  if (allMatch == lastAndState)
    {
      return;
    }

  lastAndState = allMatch;
  cmd = allMatch ? onCmd : offCmd;

  if (!iodata || !target)
    {
      return;
    }

  std::memset(iodata->getDataPtr(), 0, iodata->getDataSize());
  std::memcpy(iodata->getDataPtr(), &cmd, sizeof(cmd));

  ret = target->setData(*iodata);
  if (ret != OK)
    {
      DAWNERR("switch: setData to target failed %d\n", ret);
    }
}
