// dawn/src/prog/onetomany.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/onetomany.hxx"

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"

using namespace dawn;

CProgOneToMany::CProgOneToMany(CDescObject &desc)
  : CProgCommon(desc)
  , input(nullptr)
  , inputId(0)
  , dataBuf(nullptr)
  , active(false)
  , registered(false)
{
}

CProgOneToMany::~CProgOneToMany()
{
  deinit();
}

int CProgOneToMany::allocOutput(SObjectId::ObjectId ioId)
{
  outputIds.push_back(ioId);
  outputs.push_back(nullptr);
  setObjectMapItem(ioId, nullptr);
  return OK;
}

int CProgOneToMany::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_ONETOMANY)
        {
          DAWNERR("onetomany: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_ONETOMANY_CFG_INPUT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("onetomany: INPUT must have one item\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              inputId = ids[0].v;
              setObjectMapItem(inputId, nullptr);
              break;
            }

          case PROG_ONETOMANY_CFG_OUTPUTS:
            {
              if (item->cfgid.s.size == 0)
                {
                  DAWNERR("onetomany: OUTPUTS must not be empty\n");
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
            DAWNERR("onetomany: unsupported cfg id %u\n", item->cfgid.s.id);
            return -EINVAL;
        }
    }

  if (inputId == 0 || outputs.empty())
    {
      DAWNERR("onetomany: input and outputs are required\n");
      return -EINVAL;
    }

  return OK;
}

int CProgOneToMany::configure()
{
  return configureDesc(getDesc());
}

int CProgOneToMany::validateShape(CIOCommon *io) const
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

int CProgOneToMany::init()
{
  input = getIO(inputId);
  if (input == nullptr || !input->isRead() || !input->isNotify())
    {
      DAWNERR("onetomany: input 0x%" PRIx32 " is not readable notify IO\n", inputId);
      return -EINVAL;
    }

  for (size_t i = 0; i < outputIds.size(); i++)
    {
      outputs[i] = getIO(outputIds[i]);
      if (outputs[i] == nullptr)
        {
          DAWNERR("onetomany: output IO 0x%" PRIx32 " not found\n", outputIds[i]);
          return -EIO;
        }

      int ret = prepareWritableTarget(outputs[i], input->getDataDim(), true);
      if (ret != OK)
        {
          DAWNERR("onetomany: output prepare failed %d\n", ret);
          return ret;
        }

      ret = validateShape(outputs[i]);
      if (ret != OK)
        {
          DAWNERR("onetomany: output 0x%" PRIx32 " incompatible\n", outputIds[i]);
          return ret;
        }
    }

  dataBuf = input->ddata_alloc(1);
  if (dataBuf == nullptr)
    {
      DAWNERR("onetomany: data allocation failed\n");
      return -ENOMEM;
    }

  return OK;
}

int CProgOneToMany::deinit()
{
  doStop();
  delete dataBuf;
  dataBuf = nullptr;
  input = nullptr;
  inputId = 0;
  outputs.clear();
  outputIds.clear();
  return OK;
}

int CProgOneToMany::ioNotifierCb(void *priv, io_ddata_t *data)
{
  CProgOneToMany *self = static_cast<CProgOneToMany *>(priv);

  if (self == nullptr || !self->active || data == nullptr)
    {
      return OK;
    }

  self->writeOutputs(data);
  return OK;
}

int CProgOneToMany::doStart()
{
  if (!registered)
    {
      int ret = input->setNotifier(ioNotifierCb, 0, this);
      if (ret != OK)
        {
          DAWNERR("onetomany: setNotifier failed %d\n", ret);
          return ret;
        }
      registered = true;
    }

  active = true;
  if (input->getData(*dataBuf, 1) == OK)
    {
      writeOutputs(dataBuf);
    }
  return OK;
}

int CProgOneToMany::doStop()
{
  if (registered && input != nullptr)
    {
      input->setNotifier(nullptr, 0, nullptr);
      registered = false;
    }

  active = false;
  return OK;
}

bool CProgOneToMany::hasThread() const
{
  return false;
}

void CProgOneToMany::writeOutputs(io_ddata_t *data)
{
  for (auto output : outputs)
    {
      int ret = output->setData(*data);
      if (ret != OK)
        {
          DAWNERR("onetomany: output setData failed %d\n", ret);
        }
    }
}
