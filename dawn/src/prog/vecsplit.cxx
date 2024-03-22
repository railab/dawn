// dawn/src/prog/vecsplit.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/vecsplit.hxx"

#include <cstring>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"

using namespace dawn;

CProgVecSplit::CProgVecSplit(CDescObject &desc)
  : CProgCommon(desc)
  , source(nullptr)
  , sourceId(0)
  , sourceData(nullptr)
  , active(false)
  , registered(false)
{
}

CProgVecSplit::~CProgVecSplit()
{
  deinit();
}

int CProgVecSplit::allocOutput(SObjectId::ObjectId ioId)
{
  SVecOutput output;

  output.io = nullptr;
  output.ioId = ioId;
  output.data = nullptr;
  output.offset = 0;
  outputs.push_back(output);
  setObjectMapItem(ioId, nullptr);
  return OK;
}

int CProgVecSplit::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_VECSPLIT)
        {
          DAWNERR("vecsplit: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_VECSPLIT_CFG_SOURCE:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("vecsplit: SOURCE must have one item\n");
                  return -EINVAL;
                }

              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              sourceId = ids[0].v;
              setObjectMapItem(sourceId, nullptr);
              break;
            }

          case PROG_VECSPLIT_CFG_OUTPUTS:
            {
              if (item->cfgid.s.size == 0)
                {
                  DAWNERR("vecsplit: OUTPUTS must not be empty\n");
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
            {
              DAWNERR("vecsplit: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (sourceId == 0 || outputs.empty())
    {
      DAWNERR("vecsplit: source and outputs are required\n");
      return -EINVAL;
    }

  return OK;
}

int CProgVecSplit::configure()
{
  return configureDesc(getDesc());
}

int CProgVecSplit::init()
{
  size_t dim = 0;
  int ret;

  source = getIO(sourceId);
  if (source == nullptr || !source->isRead())
    {
      DAWNERR("vecsplit: source 0x%" PRIx32 " not readable\n", sourceId);
      return -EIO;
    }

  for (auto &output : outputs)
    {
      output.io = getIO(output.ioId);
      if (output.io == nullptr)
        {
          DAWNERR("vecsplit: output IO 0x%" PRIx32 " not found\n", output.ioId);
          return -EIO;
        }

      if (output.io->getDataDim() == 0)
        {
          ret = prepareWritableTarget(output.io, 1, true);
          if (ret != OK)
            {
              DAWNERR("vecsplit: output prepare failed %d\n", ret);
              return ret;
            }
        }

      if (output.io->getDtype() != source->getDtype())
        {
          DAWNERR("vecsplit: output 0x%" PRIx32 " dtype mismatch\n", output.ioId);
          return -EINVAL;
        }

      output.offset = dim;
      dim += output.io->getDataDim();
      output.data = output.io->ddata_alloc(1);
      if (output.data == nullptr)
        {
          return -ENOMEM;
        }
    }

  if (dim > source->getDataDim())
    {
      DAWNERR("vecsplit: outputs need %zu elements, source has %zu\n", dim, source->getDataDim());
      return -EINVAL;
    }

  sourceData = source->ddata_alloc(1);
  if (sourceData == nullptr)
    {
      return -ENOMEM;
    }

  return OK;
}

int CProgVecSplit::deinit()
{
  doStop();

  delete sourceData;
  sourceData = nullptr;
  for (auto &output : outputs)
    {
      delete output.data;
      output.data = nullptr;
    }

  outputs.clear();
  source = nullptr;
  sourceId = 0;

  return OK;
}

int CProgVecSplit::ioNotifierCb(void *priv, io_ddata_t *data)
{
  CProgVecSplit *self = static_cast<CProgVecSplit *>(priv);

  if (self != nullptr && self->active)
    {
      self->updateOutputs(data);
    }

  return OK;
}

int CProgVecSplit::doStart()
{
  int ret;

  if (!registered && source->isNotify())
    {
      ret = source->setNotifier(ioNotifierCb, 0, this);
      if (ret != OK)
        {
          DAWNERR("vecsplit: setNotifier failed %d\n", ret);
          return ret;
        }

      registered = true;
    }

  active = true;
  updateOutputs(nullptr);
  return OK;
}

int CProgVecSplit::doStop()
{
  if (registered && source != nullptr && source->isNotify())
    {
      source->setNotifier(nullptr, 0, nullptr);
      registered = false;
    }

  active = false;
  return OK;
}

bool CProgVecSplit::hasThread() const
{
  return false;
}

void CProgVecSplit::updateOutputs(io_ddata_t *data)
{
  int ret;

  if (source == nullptr || sourceData == nullptr)
    {
      return;
    }

  if (data != nullptr)
    {
      std::memcpy(sourceData->getDataPtr(), data->getDataPtr(), sourceData->getDataSize());
    }
  else if (source->getData(*sourceData, 1) != OK)
    {
      return;
    }

  for (auto &output : outputs)
    {
      std::memcpy(output.data->getDataPtr(),
                  static_cast<uint8_t *>(sourceData->getDataPtr()) +
                    output.offset * source->getDtypeSize(),
                  output.data->getDataSize());
      ret = output.io->setData(*output.data);
      if (ret != OK)
        {
          DAWNERR("vecsplit: output setData failed %d\n", ret);
        }
    }
}
