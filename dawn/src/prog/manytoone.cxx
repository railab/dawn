// dawn/src/prog/manytoone.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/manytoone.hxx"

#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"

using namespace dawn;

CProgManyToOne::CProgManyToOne(CDescObject &desc)
  : CProgCommon(desc)
  , output(nullptr)
  , outputId(0)
  , dataBuf(nullptr)
  , active(false)
  , registered(false)
{
}

CProgManyToOne::~CProgManyToOne()
{
  deinit();
}

int CProgManyToOne::allocInput(SObjectId::ObjectId ioId)
{
  SInput input;

  input.owner = this;
  input.io = nullptr;
  input.ioId = ioId;
  inputs.push_back(input);
  setObjectMapItem(ioId, nullptr);
  return OK;
}

int CProgManyToOne::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_MANYTOONE)
        {
          DAWNERR("manytoone: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_MANYTOONE_CFG_INPUTS:
            {
              if (item->cfgid.s.size == 0)
                {
                  DAWNERR("manytoone: INPUTS must not be empty\n");
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

          case PROG_MANYTOONE_CFG_OUTPUT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("manytoone: OUTPUT must have one item\n");
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              outputId = ids[0].v;
              setObjectMapItem(outputId, nullptr);
              break;
            }

          default:
            DAWNERR("manytoone: unsupported cfg id %u\n", item->cfgid.s.id);
            return -EINVAL;
        }
    }

  if (inputs.empty() || outputId == 0)
    {
      DAWNERR("manytoone: inputs and output are required\n");
      return -EINVAL;
    }

  return OK;
}

int CProgManyToOne::configure()
{
  return configureDesc(getDesc());
}

int CProgManyToOne::validateShape(CIOCommon *io) const
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

int CProgManyToOne::init()
{
  output = getIO(outputId);
  if (output == nullptr)
    {
      DAWNERR("manytoone: output IO 0x%" PRIx32 " not found\n", outputId);
      return -EIO;
    }

  for (auto &input : inputs)
    {
      input.io = getIO(input.ioId);
      if (input.io == nullptr)
        {
          DAWNERR("manytoone: input IO 0x%" PRIx32 " not found\n", input.ioId);
          return -EIO;
        }
    }

  int ret = prepareWritableTarget(output, inputs[0].io->getDataDim(), true);
  if (ret != OK)
    {
      DAWNERR("manytoone: output prepare failed %d\n", ret);
      return ret;
    }

  for (auto &input : inputs)
    {
      ret = validateShape(input.io);
      if (ret != OK)
        {
          DAWNERR("manytoone: input 0x%" PRIx32 " incompatible\n", input.ioId);
          return ret;
        }
    }

  dataBuf = output->ddata_alloc(1);
  if (dataBuf == nullptr)
    {
      DAWNERR("manytoone: data allocation failed\n");
      return -ENOMEM;
    }

  return OK;
}

int CProgManyToOne::deinit()
{
  doStop();
  delete dataBuf;
  dataBuf = nullptr;
  output = nullptr;
  outputId = 0;
  inputs.clear();
  return OK;
}

int CProgManyToOne::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SInput *input = static_cast<SInput *>(priv);

  if (input == nullptr || input->owner == nullptr || !input->owner->active || data == nullptr)
    {
      return OK;
    }

  int ret = input->owner->output->setData(*data);
  if (ret != OK)
    {
      DAWNERR("manytoone: output setData failed %d\n", ret);
      return ret;
    }

  return OK;
}

int CProgManyToOne::doStart()
{
  if (!registered)
    {
      for (auto &input : inputs)
        {
          int ret = input.io->setNotifier(ioNotifierCb, 0, &input);
          if (ret != OK)
            {
              DAWNERR("manytoone: setNotifier failed for 0x%" PRIx32 ": %d\n", input.ioId, ret);
              return ret;
            }
        }
      registered = true;
    }

  active = true;
  writeInput(0);
  return OK;
}

int CProgManyToOne::doStop()
{
  if (registered)
    {
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

bool CProgManyToOne::hasThread() const
{
  return false;
}

void CProgManyToOne::writeInput(size_t index)
{
  if (index >= inputs.size() || dataBuf == nullptr || output == nullptr)
    {
      return;
    }

  if (inputs[index].io->getData(*dataBuf, 1) != OK)
    {
      return;
    }

  int ret = output->setData(*dataBuf);
  if (ret != OK)
    {
      DAWNERR("manytoone: output setData failed %d\n", ret);
    }
}
