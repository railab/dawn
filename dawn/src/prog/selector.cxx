// dawn/src/prog/selector.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/selector.hxx"

#include <cstring>
#include <new>

#include "dawn/debug.hxx"
#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"
#include "dawn/io/sdata.hxx"

using namespace dawn;

CProgSelector::CProgSelector(CDescObject &desc)
  : CProgCommon(desc)
  , ctrlIo(nullptr)
  , target(nullptr)
  , ctrlId(0)
  , targetId(0)
  , iodata(nullptr)
  , currentIndex(0)
  , active(false)
  , registered(false)
{
}

CProgSelector::~CProgSelector()
{
  deinit();
}

int CProgSelector::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  const SObjectId::UObjectId *ids;
  size_t offset;
  size_t ii;
  size_t n;

  offset = 0;
  for (ii = 0; ii < desc.getSize(); ii++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProgCommon::PROG_CLASS_SELECTOR)
        {
          DAWNERR("selector: unsupported cfg class 0x%" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROG_SELECTOR_CFG_CONTROL:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("selector: CONTROL must have 1 entry\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              ctrlId = ids[0].v;
              setObjectMapItem(ctrlId, nullptr);
              break;
            }

          case PROG_SELECTOR_CFG_DATA:
            {
              n = static_cast<size_t>(item->cfgid.s.size);
              if (n == 0)
                {
                  DAWNERR("selector: DATA must have at least 1 entry\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              for (size_t j = 0; j < n; j++)
                {
                  dataIds.push_back(ids[j].v);
                  dataIos.push_back(nullptr);
                  SDataBind db;
                  db.owner = this;
                  db.index = j;
                  dataBinds.push_back(db);
                  setObjectMapItem(ids[j].v, nullptr);
                }
              break;
            }

          case PROG_SELECTOR_CFG_TARGET:
            {
              if (item->cfgid.s.size != 1)
                {
                  DAWNERR("selector: TARGET must have 1 entry\n");
                  return -EINVAL;
                }
              ids = reinterpret_cast<const SObjectId::UObjectId *>(item->data);
              targetId = ids[0].v;
              setObjectMapItem(targetId, nullptr);
              break;
            }

          default:
            {
              DAWNERR("selector: unsupported cfg id %u\n", item->cfgid.s.id);
              return -EINVAL;
            }
        }
    }

  if (ctrlId == 0 || dataIds.empty() || targetId == 0)
    {
      DAWNERR("selector: missing control, data, or target config\n");
      return -EINVAL;
    }

  return OK;
}

int CProgSelector::configure()
{
  return configureDesc(getDesc());
}

int CProgSelector::init()
{
  CIOCommon *io;
  size_t i;
  size_t dataDim;
  size_t dataSize;
  uint8_t dataDtype;
  int ret;

  io = getIO(ctrlId);
  if (!io)
    {
      DAWNERR("selector: control IO 0x%" PRIx32 " not found\n", ctrlId);
      return -EIO;
    }
  if (!io->isRead())
    {
      DAWNERR("selector: control IO 0x%" PRIx32 " is not readable\n", ctrlId);
      return -EINVAL;
    }
  if (io->getDtype() != SObjectId::DTYPE_UINT32 || io->getDataDim() != 1)
    {
      DAWNERR("selector: control IO 0x%" PRIx32 " must be scalar uint32\n", ctrlId);
      return -EINVAL;
    }
  ctrlIo = io;

  for (i = 0; i < dataIds.size(); i++)
    {
      io = getIO(dataIds[i]);
      if (!io)
        {
          DAWNERR("selector: data IO 0x%" PRIx32 " not found\n", dataIds[i]);
          return -EIO;
        }
      if (!io->isRead())
        {
          DAWNERR("selector: data IO 0x%" PRIx32 " is not readable\n", dataIds[i]);
          return -EINVAL;
        }
      dataIos[i] = io;
    }

  dataDim = dataIos[0]->getDataDim();
  dataSize = dataIos[0]->getDtypeSize();
  dataDtype = dataIos[0]->getDtype();

  for (i = 1; i < dataIos.size(); i++)
    {
      if (dataIos[i]->getDataDim() != dataDim || dataIos[i]->getDtypeSize() != dataSize ||
          dataIos[i]->getDtype() != dataDtype)
        {
          DAWNERR("selector: data IO 0x%" PRIx32 " shape mismatch with first data input\n",
                  dataIds[i]);
          return -EINVAL;
        }
    }

  target = getIO(targetId);
  if (!target)
    {
      DAWNERR("selector: target 0x%" PRIx32 " not found\n", targetId);
      return -EIO;
    }

  ret = prepareWritableTarget(target, dataDim, true);
  if (ret != OK)
    {
      DAWNERR("selector: target prepare failed %d\n", ret);
      return ret;
    }

  iodata = new (std::nothrow) io_ddata_t(dataSize, dataDim, 1, dataDtype);
  if (!iodata || !iodata->isAllocated())
    {
      delete iodata;
      iodata = nullptr;
      DAWNERR("selector: iodata allocation failed\n");
      return -ENOMEM;
    }

  return OK;
}

int CProgSelector::deinit()
{
  doStop();
  delete iodata;
  iodata = nullptr;
  target = nullptr;
  ctrlIo = nullptr;
  dataIos.clear();
  dataIds.clear();
  dataBinds.clear();
  return OK;
}

int CProgSelector::ctrlNotifierCb(void *priv, io_ddata_t *data)
{
  CProgSelector *sel = static_cast<CProgSelector *>(priv);
  uint32_t idx;

  if (!data || data->getItems() < 1 || !sel->active)
    {
      return OK;
    }

  std::memcpy(&idx, data->getDataPtr(), sizeof(idx));
  sel->applyRoute(idx);
  return OK;
}

int CProgSelector::dataNotifierCb(void *priv, io_ddata_t *data)
{
  SDataBind *bind = static_cast<SDataBind *>(priv);
  uint32_t index;

  (void)data;

  if (!bind->owner || !bind->owner->active)
    {
      return OK;
    }

  if (bind->owner->ctrlIo)
    {
      io_sdata_t<uint32_t, 1, 1> ctrlData;

      if (bind->owner->ctrlIo->getData(ctrlData, 1) == OK)
        {
          index = ctrlData(0);
          if (index < bind->owner->dataIos.size())
            {
              bind->owner->currentIndex = static_cast<size_t>(index);
            }
        }
    }

  // Re-route only if this data input is the currently selected one.
  if (bind->index == bind->owner->currentIndex)
    {
      bind->owner->applyRoute(static_cast<uint32_t>(bind->index));
    }

  return OK;
}

int CProgSelector::doStart()
{
  size_t i;
  int ret;
  uint32_t index;

  if (!registered)
    {
      if (ctrlIo && ctrlIo->isNotify())
        {
          ret = ctrlIo->setNotifier(ctrlNotifierCb, 0, this);
          if (ret != OK)
            {
              DAWNERR("selector: setNotifier on control failed %d\n", ret);
              return ret;
            }
        }

      // Register notifiers on data IOs that support them.
      for (i = 0; i < dataIos.size(); i++)
        {
          if (dataIos[i] && dataIos[i]->isNotify())
            {
              ret = dataIos[i]->setNotifier(dataNotifierCb, 0, &dataBinds[i]);
              if (ret != OK)
                {
                  DAWNERR("selector: setNotifier on data[%zu] failed %d\n", i, ret);
                  return ret;
                }
            }
        }

      registered = true;
    }

  active = true;

  if (ctrlIo)
    {
      io_sdata_t<uint32_t, 1, 1> ctrlData;

      if (ctrlIo->getData(ctrlData, 1) == OK)
        {
          index = ctrlData(0);
          applyRoute(index);
        }
    }

  return OK;
}

int CProgSelector::doStop()
{
  if (registered)
    {
      if (ctrlIo && ctrlIo->isNotify())
        {
          ctrlIo->setNotifier(nullptr, 0, nullptr);
        }

      for (size_t i = 0; i < dataIos.size(); i++)
        {
          if (dataIos[i] && dataIos[i]->isNotify())
            {
              dataIos[i]->setNotifier(nullptr, 0, nullptr);
            }
        }

      registered = false;
    }

  active = false;
  return OK;
}

bool CProgSelector::hasThread() const
{
  return false;
}

void CProgSelector::applyRoute(uint32_t index)
{
  int ret;

  if (index >= dataIos.size() || !dataIos[index] || !iodata || !target)
    {
      return;
    }

  currentIndex = static_cast<size_t>(index);

  ret = dataIos[index]->getData(*iodata, 1);
  if (ret != OK)
    {
      DAWNERR("selector: getData from data[%" PRIu32 "] failed %d\n", index, ret);
      return;
    }

  ret = target->setData(*iodata);
  if (ret != OK)
    {
      DAWNERR("selector: setData to target failed %d\n", ret);
    }
}
