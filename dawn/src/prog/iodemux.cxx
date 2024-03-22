// dawn/src/prog/iodemux.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/iodemux.hxx"

#include <cstring>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"

using namespace dawn;

CProgIODemux::CProgIODemux(CDescObject &desc)
  : CProgCommon(desc)
  , control(nullptr)
  , controlId(0)
  , input(nullptr)
  , inputId(0)
  , dataBuf(nullptr)
  , currentIndex(0)
  , active(false)
  , registered(false)
{
}

CProgIODemux::~CProgIODemux()
{
  deinit();
}

int CProgIODemux::allocOutput(SObjectId::ObjectId ioId)
{
  outputIds.push_back(ioId);
  outputs.push_back(nullptr);
  setObjectMapItem(ioId, nullptr);
  return OK;
}

int CProgIODemux::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_IODEMUX)
        {
          DAWNERR("iodemux: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_IODEMUX_CFG_CONTROL:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("iodemux: CONTROL must have one item\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              controlId = ids[0].v;
              setObjectMapItem(controlId, nullptr);
              break;
            }

          case PROG_IODEMUX_CFG_INPUT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("iodemux: INPUT must have one item\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              inputId = ids[0].v;
              setObjectMapItem(inputId, nullptr);
              break;
            }

          case PROG_IODEMUX_CFG_OUTPUTS:
            {
              if (item->cfgid.s.size == 0)
                {
                  DAWNERR("iodemux: OUTPUTS must not be empty\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              for (size_t j = 0; j < item->cfgid.s.size; j++)
                {
                  int ret = allocOutput(ids[j].v);
                  if (ret != OK)
                    {
                      return ret;
                    }
                }
              break;
            }

          default:
            DAWNERR("iodemux: unsupported cfg id %u\n", item->cfgid.s.id);
            return -EINVAL;
        }
    }

  if (controlId == 0 || inputId == 0 || outputs.empty())
    {
      DAWNERR("iodemux: control, input and outputs are required\n");
      return -EINVAL;
    }

  return OK;
}

int CProgIODemux::configure()
{
  return configureDesc(getDesc());
}

int CProgIODemux::validateShape(CIOCommon *io) const
{
  if (io == nullptr || input == nullptr)
    {
      return -EIO;
    }

  if (io->getDtype() != input->getDtype() || io->getDataDim() != input->getDataDim() ||
      io->isTimestamp() != input->isTimestamp() || io->getDataSize() != input->getDataSize())
    {
      return -EINVAL;
    }

  return OK;
}

int CProgIODemux::init()
{
  control = getIO(controlId);
  if (control == nullptr || !control->isRead() || !control->isNotify() ||
      control->getDtype() != SObjectId::DTYPE_UINT32 || control->getDataDim() != 1)
    {
      DAWNERR("iodemux: control 0x%" PRIx32 " must be scalar uint32 notify IO\n", controlId);
      return -EINVAL;
    }

  input = getIO(inputId);
  if (input == nullptr || !input->isRead() || !input->isNotify())
    {
      DAWNERR("iodemux: input 0x%" PRIx32 " is not readable notify IO\n", inputId);
      return -EINVAL;
    }

  for (size_t i = 0; i < outputIds.size(); i++)
    {
      outputs[i] = getIO(outputIds[i]);
      if (outputs[i] == nullptr)
        {
          DAWNERR("iodemux: output IO 0x%" PRIx32 " not found\n", outputIds[i]);
          return -EIO;
        }

      int ret = prepareWritableTarget(outputs[i], input->getDataDim(), true);
      if (ret != OK)
        {
          DAWNERR("iodemux: output prepare failed %d\n", ret);
          return ret;
        }

      ret = validateShape(outputs[i]);
      if (ret != OK)
        {
          DAWNERR("iodemux: output 0x%" PRIx32 " incompatible\n", outputIds[i]);
          return ret;
        }
    }

  dataBuf = input->ddata_alloc(1);
  if (dataBuf == nullptr)
    {
      return -ENOMEM;
    }

  return OK;
}

int CProgIODemux::deinit()
{
  doStop();
  delete dataBuf;
  dataBuf = nullptr;
  control = nullptr;
  controlId = 0;
  input = nullptr;
  inputId = 0;
  outputs.clear();
  outputIds.clear();
  return OK;
}

int CProgIODemux::controlNotifierCb(void *priv, io_ddata_t *data)
{
  CProgIODemux *self = static_cast<CProgIODemux *>(priv);
  uint32_t index;

  if (self == nullptr || !self->active || data == nullptr || data->getItems() < 1)
    {
      return OK;
    }

  std::memcpy(&index, data->getDataPtr(), sizeof(index));
  self->routeIndex(index);
  return OK;
}

int CProgIODemux::inputNotifierCb(void *priv, io_ddata_t *data)
{
  CProgIODemux *self = static_cast<CProgIODemux *>(priv);

  UNUSED(data);

  if (self == nullptr || !self->active)
    {
      return OK;
    }

  self->routeIndex(static_cast<uint32_t>(self->currentIndex));
  return OK;
}

int CProgIODemux::doStart()
{
  int ret;

  if (!registered)
    {
      ret = control->setNotifier(controlNotifierCb, 0, this);
      if (ret != OK)
        {
          DAWNERR("iodemux: control setNotifier failed %d\n", ret);
          return ret;
        }

      ret = input->setNotifier(inputNotifierCb, 0, this);
      if (ret != OK)
        {
          DAWNERR("iodemux: input setNotifier failed %d\n", ret);
          return ret;
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

int CProgIODemux::doStop()
{
  if (registered)
    {
      if (control != nullptr)
        {
          control->setNotifier(nullptr, 0, nullptr);
        }
      if (input != nullptr)
        {
          input->setNotifier(nullptr, 0, nullptr);
        }
      registered = false;
    }

  active = false;
  return OK;
}

bool CProgIODemux::hasThread() const
{
  return false;
}

void CProgIODemux::routeIndex(uint32_t index)
{
  if (index >= outputs.size() || dataBuf == nullptr)
    {
      return;
    }

  currentIndex = index;
  if (input->getData(*dataBuf, 1) != OK)
    {
      return;
    }

  int ret = outputs[index]->setData(*dataBuf);
  if (ret != OK)
    {
      DAWNERR("iodemux: output setData failed %d\n", ret);
    }
}
