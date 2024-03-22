// dawn/src/prog/vecpack.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/vecpack.hxx"

#include <cstring>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

CProgVecPack::CProgVecPack(CDescObject &desc)
  : CProgCommon(desc)
  , output(nullptr)
  , outputId(0)
  , outputData(nullptr)
  , lastOutputData(nullptr)
  , active(false)
  , registered(false)
{
}

CProgVecPack::~CProgVecPack()
{
  deinit();
}

int CProgVecPack::allocInput(SObjectId::ObjectId ioId)
{
  SVecInput input;

  input.owner = this;
  input.io = nullptr;
  input.ioId = ioId;
  input.data = nullptr;
  input.offset = 0;
  input.usesSetCallback = false;
  inputs.push_back(input);
  setObjectMapItem(ioId, nullptr);
  return OK;
}

int CProgVecPack::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_VECPACK)
        {
          DAWNERR("vecpack: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_VECPACK_CFG_INPUTS:
            {
              if (item->cfgid.s.size == 0)
                {
                  DAWNERR("vecpack: INPUTS must not be empty\n");
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

          case PROG_VECPACK_CFG_OUTPUT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("vecpack: OUTPUT must have one item\n");
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              outputId = ids[0].v;
              setObjectMapItem(outputId, nullptr);
              break;
            }

          default:
            {
              DAWNERR("vecpack: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (inputs.empty() || outputId == 0)
    {
      DAWNERR("vecpack: inputs and output are required\n");
      return -EINVAL;
    }

  return OK;
}

int CProgVecPack::configure()
{
  return configureDesc(getDesc());
}

int CProgVecPack::init()
{
  size_t dim = 0;
  int ret;

  output = getIO(outputId);
  if (output == nullptr)
    {
      DAWNERR("vecpack: output IO 0x%" PRIx32 " not found\n", outputId);
      return -EIO;
    }

  for (size_t i = 0; i < inputs.size(); i++)
    {
      inputs[i].io = getIO(inputs[i].ioId);
      if (inputs[i].io == nullptr)
        {
          DAWNERR("vecpack: input IO 0x%" PRIx32 " not found\n", inputs[i].ioId);
          return -EIO;
        }

      if (inputs[i].io->getDataDim() == 0)
        {
          ret = prepareWritableTarget(inputs[i].io, 1, false);
          if (ret != OK)
            {
              DAWNERR("vecpack: input prepare failed %d\n", ret);
              return ret;
            }
        }

      if (!inputs[i].io->isRead() || inputs[i].io->getDtype() != output->getDtype())
        {
          DAWNERR("vecpack: input 0x%" PRIx32 " incompatible with output 0x%" PRIx32 "\n",
                  inputs[i].ioId,
                  outputId);
          return -EINVAL;
        }

      inputs[i].offset = dim;
      dim += inputs[i].io->getDataDim();
      inputs[i].data = inputs[i].io->ddata_alloc(1);
      if (inputs[i].data == nullptr)
        {
          DAWNERR("vecpack: input data allocation failed\n");
          return -ENOMEM;
        }
    }

  ret = prepareWritableTarget(output, dim, true);
  if (ret != OK)
    {
      DAWNERR("vecpack: output prepare failed %d\n", ret);
      return ret;
    }

  if (output->getDataDim() != dim)
    {
      DAWNERR(
        "vecpack: output dimension mismatch expected %zu got %zu\n", dim, output->getDataDim());
      return -EINVAL;
    }

  outputData = output->ddata_alloc(1);
  lastOutputData = output->ddata_alloc(1);
  if (outputData == nullptr || lastOutputData == nullptr)
    {
      DAWNERR("vecpack: output data allocation failed\n");
      return -ENOMEM;
    }

  std::memset(lastOutputData->getDataPtr(), 0xff, lastOutputData->getDataSize());
  return OK;
}

int CProgVecPack::deinit()
{
  doStop();

  for (auto &input : inputs)
    {
      delete input.data;
      input.data = nullptr;
    }

  inputs.clear();
  delete outputData;
  outputData = nullptr;
  delete lastOutputData;
  lastOutputData = nullptr;
  output = nullptr;
  outputId = 0;
  return OK;
}

int CProgVecPack::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SVecInput *input = static_cast<SVecInput *>(priv);

  if (input == nullptr || input->owner == nullptr || !input->owner->active)
    {
      return OK;
    }

  if (data != nullptr && input->data != nullptr)
    {
      std::memcpy(input->data->getDataPtr(), data->getDataPtr(), input->data->getDataSize());
    }

  input->owner->updateOutput();
  return OK;
}

void CProgVecPack::ioSetCb(CIOVirt *io, void *priv)
{
  SVecInput *input = static_cast<SVecInput *>(priv);

  UNUSED(io);

  if (input == nullptr || input->owner == nullptr || !input->owner->active)
    {
      return;
    }

  input->owner->updateOutput();
}

int CProgVecPack::doStart()
{
  int ret;

  if (!registered)
    {
      for (auto &input : inputs)
        {
          if (input.io->isNotify())
            {
              ret = input.io->setNotifier(ioNotifierCb, 0, &input);
              if (ret != OK)
                {
                  DAWNERR("vecpack: setNotifier failed for 0x%" PRIx32 ": %d\n", input.ioId, ret);
                  return ret;
                }

              continue;
            }

          if (input.io->getCls() != CIOCommon::IO_CLASS_VIRT)
            {
              continue;
            }

          ret = reinterpret_cast<CIOVirt *>(input.io)->setCallbackSet(ioSetCb, &input);
          if (ret != OK)
            {
              DAWNERR("vecpack: setCallbackSet failed for 0x%" PRIx32 ": %d\n", input.ioId, ret);
              return ret;
            }

          input.usesSetCallback = true;
        }

      registered = true;
    }

  active = true;
  updateOutput();
  return OK;
}

int CProgVecPack::doStop()
{
  if (registered)
    {
      for (auto &input : inputs)
        {
          if (input.io == nullptr)
            {
              continue;
            }

          if (input.io->isNotify())
            {
              input.io->setNotifier(nullptr, 0, nullptr);
            }
          else if (input.usesSetCallback && input.io->getCls() == CIOCommon::IO_CLASS_VIRT)
            {
              reinterpret_cast<CIOVirt *>(input.io)->setCallbackSet(nullptr, nullptr);
              input.usesSetCallback = false;
            }
        }

      registered = false;
    }

  active = false;
  return OK;
}

bool CProgVecPack::hasThread() const
{
  return false;
}

void CProgVecPack::updateOutput()
{
  int ret;

  if (outputData == nullptr || lastOutputData == nullptr || output == nullptr)
    {
      return;
    }

  for (auto &input : inputs)
    {
      if (input.io->getData(*input.data, 1) != OK)
        {
          return;
        }

      std::memcpy(static_cast<uint8_t *>(outputData->getDataPtr()) +
                    input.offset * input.io->getDtypeSize(),
                  input.data->getDataPtr(),
                  input.data->getDataSize());
    }

  if (std::memcmp(
        outputData->getDataPtr(), lastOutputData->getDataPtr(), outputData->getDataSize()) == 0)
    {
      return;
    }

  std::memcpy(lastOutputData->getDataPtr(), outputData->getDataPtr(), outputData->getDataSize());
  ret = output->setData(*outputData);
  if (ret != OK)
    {
      DAWNERR("vecpack: output setData failed %d\n", ret);
    }
}
