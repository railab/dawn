// dawn/src/prog/iomux.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/iomux.hxx"

#include <cstring>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"

using namespace dawn;

CProgIOMux::CProgIOMux(CDescObject &desc)
  : CProgCommon(desc)
  , control(nullptr)
  , controlId(0)
  , output(nullptr)
  , outputId(0)
  , dataBuf(nullptr)
  , currentIndex(0)
  , active(false)
  , registered(false)
{
}

CProgIOMux::~CProgIOMux()
{
  deinit();
}

int CProgIOMux::allocInput(SObjectId::ObjectId ioId)
{
  SInput input;

  input.owner = this;
  input.io = nullptr;
  input.ioId = ioId;
  input.index = inputs.size();
  inputs.push_back(input);
  setObjectMapItem(ioId, nullptr);
  return OK;
}

int CProgIOMux::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_IOMUX)
        {
          DAWNERR("iomux: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_IOMUX_CFG_CONTROL:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("iomux: CONTROL must have one item\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              controlId = ids[0].v;
              setObjectMapItem(controlId, nullptr);
              break;
            }

          case PROG_IOMUX_CFG_INPUTS:
            {
              if (item->cfgid.s.size == 0)
                {
                  DAWNERR("iomux: INPUTS must not be empty\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              for (size_t j = 0; j < item->cfgid.s.size; j++)
                {
                  int ret = allocInput(ids[j].v);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }
              break;
            }

          case PROG_IOMUX_CFG_OUTPUT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("iomux: OUTPUT must have one item\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              outputId = ids[0].v;
              setObjectMapItem(outputId, nullptr);
              break;
            }

          default:
            DAWNERR("iomux: unsupported cfg id %u\n", item->cfgid.s.id);
            return -EINVAL;
        }
    }

  if (controlId == 0 || inputs.empty() || outputId == 0)
    {
      DAWNERR("iomux: control, inputs and output are required\n");
      return -EINVAL;
    }

  return OK;
}

int CProgIOMux::configure()
{
  return configureDesc(getDesc());
}

int CProgIOMux::validateShape(CIOCommon *io) const
{
  if (io == nullptr || output == nullptr)
    {
      return -EIO;
    }

  if (!io->isRead() || !io->isNotify() || io->getDtype() != output->getDtype() ||
      io->getDataDim() != output->getDataDim() || io->isTimestamp() != output->isTimestamp() ||
      io->getDataSize() != output->getDataSize())
    {
      return -EINVAL;
    }

  return OK;
}

int CProgIOMux::init()
{
  control = getIO(controlId);
  if (control == nullptr || !control->isRead() || !control->isNotify() ||
      control->getDtype() != SObjectId::DTYPE_UINT32 || control->getDataDim() != 1)
    {
      DAWNERR("iomux: control 0x%" PRIx32 " must be scalar uint32 notify IO\n", controlId);
      return -EINVAL;
    }

  output = getIO(outputId);
  if (output == nullptr)
    {
      DAWNERR("iomux: output IO 0x%" PRIx32 " not found\n", outputId);
      return -EIO;
    }

  for (auto &input : inputs)
    {
      input.io = getIO(input.ioId);
      if (input.io == nullptr)
        {
          DAWNERR("iomux: input IO 0x%" PRIx32 " not found\n", input.ioId);
          return -EIO;
        }
    }

  int ret = prepareWritableTarget(output, inputs[0].io->getDataDim(), true);
  if (ret != OK)
    {
      DAWNERR("iomux: output prepare failed %d\n", ret);
      return ret;
    }

  for (auto &input : inputs)
    {
      ret = validateShape(input.io);
      if (ret != OK)
        {
          DAWNERR("iomux: input 0x%" PRIx32 " incompatible\n", input.ioId);
          return ret;
        }
    }

  dataBuf = output->ddata_alloc(1);
  if (dataBuf == nullptr)
    {
      return -ENOMEM;
    }

  return OK;
}

int CProgIOMux::deinit()
{
  doStop();
  delete dataBuf;
  dataBuf = nullptr;
  control = nullptr;
  controlId = 0;
  output = nullptr;
  outputId = 0;
  inputs.clear();
  return OK;
}

int CProgIOMux::controlNotifierCb(void *priv, io_ddata_t *data)
{
  CProgIOMux *self = static_cast<CProgIOMux *>(priv);
  uint32_t index;

  if (self == nullptr || !self->active || data == nullptr || data->getItems() < 1)
    {
      return OK;
    }

  std::memcpy(&index, data->getDataPtr(), sizeof(index));
  self->routeIndex(index);
  return OK;
}

int CProgIOMux::inputNotifierCb(void *priv, io_ddata_t *data)
{
  SInput *input = static_cast<SInput *>(priv);

  UNUSED(data);

  if (input == nullptr || input->owner == nullptr || !input->owner->active)
    {
      return OK;
    }

  if (input->index == input->owner->currentIndex)
    {
      input->owner->routeIndex(static_cast<uint32_t>(input->index));
    }

  return OK;
}

int CProgIOMux::doStart()
{
  int ret;

  if (!registered)
    {
      ret = control->setNotifier(controlNotifierCb, 0, this);
      if (ret != OK)
        {
          DAWNERR("iomux: control setNotifier failed %d\n", ret);
          return ret;
        }

      for (auto &input : inputs)
        {
          ret = input.io->setNotifier(inputNotifierCb, 0, &input);
          if (ret != OK)
            {
              DAWNERR("iomux: input setNotifier failed for 0x%" PRIx32 ": %d\n", input.ioId, ret);
              return ret;
            }
        }

      registered = true;
    }

  active = true;
  io_sdata_t<uint32_t, 1, 1> ctrlData;
  if (control->getData(ctrlData, 1) == OK)
    {
      routeIndex(ctrlData(0));
    }
  return OK;
}

int CProgIOMux::doStop()
{
  if (registered)
    {
      if (control != nullptr)
        {
          control->setNotifier(nullptr, 0, nullptr);
        }
      for (auto &input : inputs)
        {
          if (input.io != nullptr)
            {
              input.io->setNotifier(nullptr, 0, nullptr);
            }
        }
      registered = false;
    }

  active = false;
  return OK;
}

bool CProgIOMux::hasThread() const
{
  return false;
}

void CProgIOMux::routeIndex(uint32_t index)
{
  if (index >= inputs.size() || dataBuf == nullptr || output == nullptr)
    {
      return;
    }

  currentIndex = index;
  if (inputs[index].io->getData(*dataBuf, 1) != OK)
    {
      return;
    }

  int ret = output->setData(*dataBuf);
  if (ret != OK)
    {
      DAWNERR("iomux: output setData failed %d\n", ret);
    }
}
