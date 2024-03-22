// dawn/src/proto/nimble/prph_bas.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/prph_bas.hxx"

#include <cstring>
#include <errno.h>
#include <new>

#include "dawn/io/common.hxx"

extern "C"
{
#include "services/bas/ble_svc_bas.h"
}

using namespace dawn;

int CProtoNimblePrphBas::notifierCb(void *priv, io_ddata_t *data)
{
  SPrphNotiferCb *ncb = static_cast<SPrphNotiferCb *>(priv);

  if (ncb == nullptr)
    {
      DAWNERR("NULL ncb pointer in BAS notifier\n");
      return -EINVAL;
    }

  if (data == nullptr)
    {
      DAWNERR("NULL data pointer in BAS notifier\n");
      return -EINVAL;
    }

  if (ncb->data == nullptr)
    {
      DAWNERR("NULL BAS data buffer in notifier\n");
      return -EINVAL;
    }

  std::memcpy(ncb->data->getDataPtr(), data->getDataPtr(), ncb->data->getDataSize());

  ble_svc_bas_battery_level_set(*static_cast<uint8_t *>(ncb->data->getDataPtr()));

  return OK;
}

void CProtoNimblePrphBas::allocObject(const SObjectId::UObjectId &obj)
{
  DAWNINFO("allocate object 0x%" PRIx32 "\n", obj.v);

  // Allocate object in map

  cb->regObject(obj.v);

  vio.push_back(obj.v);
}

// Assumption: Configuration item passed by caller is a valid type supported by
// this class.
void CProtoNimblePrphBas::configureDesc(const SObjectCfg::SObjectCfgItem *item)
{
  if (item->cfgid.s.size != 1)
    {
      DAWNERR("Invalid BAS config size: %d\n", item->cfgid.s.size);
      return;
    }

  const auto *tmp = reinterpret_cast<const SProtoNimblePrphIOBindBas *>(&item->data);

  // Allocate object

  allocObject(tmp->objid);
}

CProtoNimblePrphBas::CProtoNimblePrphBas(const SObjectCfg::SObjectCfgItem *desc_,
                                         IProtoNimblePrphCb *cb_)
  : IProtoNimblePrphService(desc_, cb_)
  , ncb(nullptr)
{
  // Service ID not assigned yet

  id = -1;
}

CProtoNimblePrphBas::~CProtoNimblePrphBas()
{
  deinit();
}

int CProtoNimblePrphBas::init()
{
  // Configure object

  configureDesc(desc);

  // Battery Service

  ble_svc_bas_init();

  // Register servie in peripheral handler

  id = cb->serviceRegister(nullptr);
  if (id < 0)
    {
      DAWNERR("BAS service registration failed\n");
      return -EIO;
    }

  return OK;
}

int CProtoNimblePrphBas::deinit()
{
  if (ncb != nullptr)
    {
#ifdef CONFIG_DAWN_IO_NOTIFY
      if (ncb->io != nullptr && ncb->io->isNotify())
        {
          ncb->io->setNotifier(nullptr, 0, nullptr);
        }
#endif

      delete ncb->data;
      delete ncb;
      ncb = nullptr;
    }

  return OK;
}

int CProtoNimblePrphBas::bindObject()
{
  CIOCommon *io;
  int ret;

  if (ncb != nullptr)
    {
      return OK;
    }

  if (vio.size() != 1)
    {
      DAWNERR("BAS requires exactly one battery-level IO\n");
      return -EINVAL;
    }

  io = cb->getObject(vio[0]);
  if (io == nullptr)
    {
      DAWNERR("BAS IO not found\n");
      return -EIO;
    }

  if (io->isSeekable())
    {
      DAWNERR("seekable IO not supported by BAS (objid=0x%08" PRIx32 ")\n", io->getIdV());
      return -ENOTSUP;
    }

  if (!io->isNotify())
    {
      DAWNERR("BAS requires notify-capable IO (objid=0x%08" PRIx32 ")\n", io->getIdV());
      return -ENOTSUP;
    }

  if (io->getDataSize() < sizeof(uint8_t))
    {
      DAWNERR("BAS IO data size too small: %zu\n", io->getDataSize());
      return -EINVAL;
    }

  ncb = new (std::nothrow) SPrphNotiferCb();
  if (ncb == nullptr)
    {
      DAWNERR("BAS notifier allocation failed\n");
      return -ENOMEM;
    }

  ncb->io = io;
  ncb->handle = 0;
  ncb->data = io->ddata_alloc(1);
  if (ncb->data == nullptr)
    {
      DAWNERR("BAS data allocation failed\n");
      delete ncb;
      ncb = nullptr;
      return -ENOMEM;
    }

  ret = io->setNotifier(notifierCb, 0, ncb);
  if (ret < 0)
    {
      DAWNERR("ERROR: set notifier failed for BAS objId = 0x%" PRIx32 "\n", vio[0]);
      delete ncb->data;
      delete ncb;
      ncb = nullptr;
      return ret;
    }

  return OK;
}

int CProtoNimblePrphBas::updateBatteryLevel()
{
  uint8_t level;
  int ret;

  if (ncb == nullptr || ncb->io == nullptr || ncb->data == nullptr)
    {
      return -EINVAL;
    }

  ret = ncb->io->getData(*ncb->data, 1);
  if (ret < 0)
    {
      return ret;
    }

  level = *static_cast<uint8_t *>(ncb->data->getDataPtr());
  return ble_svc_bas_battery_level_set(level);
}

int CProtoNimblePrphBas::start()
{
  int ret;

  ret = bindObject();
  if (ret != OK)
    {
      return ret;
    }

  ret = updateBatteryLevel();
  if (ret != OK)
    {
      return ret;
    }

  // Signal start to peripheral handler

  return cb->startService(id);
}

int CProtoNimblePrphBas::stop()
{
  // Signal stop to peripheral handler

  return cb->stopService(id);
}
