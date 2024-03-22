// dawn/src/prog/bitpack.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/bitpack.hxx"

#include <algorithm>
#include <cstring>
#include <new>

#include "bitwise.hxx"
#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;
using namespace dawn::prog;

static const size_t INPUT_WORDS = 2; ///< Words per input (ioId + bit).

CProgBitPack::CProgBitPack(CDescObject &desc)
  : CProgCommon(desc)
  , output(nullptr)
  , outputId(0)
  , outputData(nullptr)
  , active(false)
  , registered(false)
{
}

CProgBitPack::~CProgBitPack()
{
  deinit();
}

int CProgBitPack::allocInput(SObjectId::ObjectId ioId, uint32_t bit)
{
  SBitInput inp;

  inp.owner = this;
  inp.io = nullptr;
  inp.ioId = ioId;
  inp.bit = bit;
  inp.currentData = nullptr;

  inputs.push_back(inp);
  setObjectMapItem(ioId, nullptr);
  return OK;
}

int CProgBitPack::configureDesc(const CDescObject &desc)
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

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_BITPACK)
        {
          DAWNERR("bitpack: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_BITPACK_CFG_INPUTS:
            {
              n = static_cast<size_t>(item->cfgid.s.size);
              if (n == 0 || n % INPUT_WORDS != 0)
                {
                  DAWNERR("bitpack: invalid INPUTS size %zu\n", n);
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

          case PROG_BITPACK_CFG_OUTPUT:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("bitpack: OUTPUT must have 1 entry\n");
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              outputId = ids[0].v;
              setObjectMapItem(outputId, nullptr);
              break;
            }

          default:
            {
              DAWNERR("bitpack: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (inputs.empty())
    {
      DAWNERR("bitpack: at least one input required\n");
      return -EINVAL;
    }

  if (outputId == 0)
    {
      DAWNERR("bitpack: output IO not configured\n");
      return -EINVAL;
    }

  return OK;
}

int CProgBitPack::configure()
{
  return configureDesc(getDesc());
}

int CProgBitPack::init()
{
  CIOCommon *io;
  size_t i;
  size_t requiredBits;
  size_t outputBitsPerElement;
  size_t outputDim;
  int ret;

  requiredBits = 0;
  for (i = 0; i < inputs.size(); i++)
    {
      io = getIO(inputs[i].ioId);
      if (!io)
        {
          DAWNERR("bitpack: input IO 0x%" PRIx32 " not found\n", inputs[i].ioId);
          return -EIO;
        }

      if (!io->isRead())
        {
          DAWNERR("bitpack: input 0x%" PRIx32 " is not readable\n", inputs[i].ioId);
          return -EINVAL;
        }

      if (!isBitwiseDtype(io->getDtype()))
        {
          DAWNERR(
            "bitpack: input 0x%" PRIx32 " unsupported dtype %u\n", inputs[i].ioId, io->getDtype());
          return -EINVAL;
        }

      inputs[i].io = io;
      inputs[i].currentData = inputs[i].io->ddata_alloc(1);
      if (inputs[i].currentData == nullptr)
        {
          DAWNERR("bitpack: currentData allocation failed for input %zu\n", i);
          return -ENOMEM;
        }

      requiredBits =
        std::max(requiredBits, static_cast<size_t>(inputs[i].bit) + getLogicalBits(inputs[i].io));
    }

  io = getIO(outputId);
  if (!io)
    {
      DAWNERR("bitpack: output IO 0x%" PRIx32 " not found\n", outputId);
      return -EIO;
    }
  output = io;

  if (!isBitwiseDtype(output->getDtype()))
    {
      DAWNERR("bitpack: output 0x%" PRIx32 " unsupported dtype %u\n", outputId, output->getDtype());
      return -EINVAL;
    }

  outputBitsPerElement = getLogicalElementBits(output->getDtype(), output->getDtypeSize());
  outputDim = (requiredBits + outputBitsPerElement - 1) / outputBitsPerElement;
  if (outputDim == 0)
    {
      outputDim = 1;
    }

  ret = prepareWritableTarget(output, outputDim, true);
  if (ret != OK)
    {
      DAWNERR("bitpack: output target prepare failed %d\n", ret);
      return ret;
    }

  outputData = output->ddata_alloc(1);
  if (outputData == nullptr)
    {
      DAWNERR("bitpack: outputData allocation failed\n");
      return -ENOMEM;
    }

  return OK;
}

int CProgBitPack::deinit()
{
  doStop();

  for (size_t i = 0; i < inputs.size(); i++)
    {
      delete inputs[i].currentData;
      inputs[i].currentData = nullptr;
    }

  inputs.clear();

  delete outputData;
  outputData = nullptr;
  output = nullptr;
  outputId = 0;
  return OK;
}

int CProgBitPack::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SBitInput *inp = static_cast<SBitInput *>(priv);

  if (data && data->getItems() >= 1 && inp->currentData)
    {
      std::memcpy(
        inp->currentData->getDataPtr(), data->getDataPtr(), inp->currentData->getDataSize());
    }

  if (inp->owner && inp->owner->active)
    {
      inp->owner->updateOutput();
    }

  return OK;
}

int CProgBitPack::doStart()
{
  size_t i;
  int ret;

  if (registered)
    {
      active = true;
      updateOutput();
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
          DAWNERR("bitpack: setNotifier failed on input %zu: %d\n", i, ret);
          return ret;
        }
    }

  registered = true;

  for (i = 0; i < inputs.size(); i++)
    {
      ret = inputs[i].io->getData(*inputs[i].currentData, 1);
      if (ret != OK)
        {
          DAWNERR("bitpack: initial getData failed on input %zu: %d\n", i, ret);
          return ret;
        }
    }

  updateOutput();
  return OK;
}

int CProgBitPack::doStop()
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

bool CProgBitPack::hasThread() const
{
  return false;
}

void CProgBitPack::updateOutput()
{
  size_t i;
  size_t bit;
  int ret;

  if (output == nullptr || outputData == nullptr)
    {
      return;
    }

  std::memset(outputData->getDataPtr(), 0, outputData->getDataSize());

  for (i = 0; i < inputs.size(); i++)
    {
      if (inputs[i].currentData == nullptr)
        {
          continue;
        }

      inputs[i].io->getData(*inputs[i].currentData, 1);

      for (bit = 0; bit < getLogicalBits(inputs[i].io); bit++)
        {
          if (readLogicalBit(inputs[i].io, inputs[i].currentData->getDataPtr(), bit))
            {
              writeLogicalBit(
                output, outputData->getDataPtr(), static_cast<size_t>(inputs[i].bit) + bit, true);
            }
        }
    }

  ret = output->setData(*outputData);
  if (ret != OK)
    {
      DAWNERR("bitpack: setData on output failed %d\n", ret);
    }
}
