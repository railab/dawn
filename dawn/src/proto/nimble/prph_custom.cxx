// dawn/src/proto/nimble/prph_custom.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/prph_custom.hxx"

#include <cstdlib>
#include <cstring>
#include <new>

#include "host/ble_hs_mbuf.h"
#include "host/ble_uuid.h"
#include "os/os_mbuf.h"

using namespace dawn;

#ifdef CONFIG_DAWN_IO_NOTIFY
int CProtoNimblePrphCustom::notifierCb(void *priv, io_ddata_t *data)
{
  SCustomCharCb *ccb = static_cast<SCustomCharCb *>(priv);

  if (ccb == nullptr || ccb->io == nullptr || ccb->data == nullptr)
    {
      return -EINVAL;
    }

  std::memcpy(ccb->data->getDataPtr(), data->getDataPtr(), ccb->data->getDataSize());
  ble_gatts_chr_updated(ccb->handle);
  return OK;
}
#endif

int CProtoNimblePrphCustom::callback(uint16_t conn_handle,
                                     uint16_t attr_handle,
                                     struct ble_gatt_access_ctxt *ctxt,
                                     void *arg)
{
  SCustomCharCb *ccb = static_cast<SCustomCharCb *>(arg);
  int ret;

  if (ccb == nullptr || ccb->io == nullptr || ccb->data == nullptr)
    {
      return BLE_ATT_ERR_UNLIKELY;
    }

  switch (ctxt->op)
    {
      case BLE_GATT_ACCESS_OP_READ_CHR:
        {
          ret = ccb->io->getData(*ccb->data, 1);
          if (ret < 0)
            {
              return BLE_ATT_ERR_UNLIKELY;
            }

          ret = os_mbuf_append(ctxt->om, ccb->data->getDataPtr(), ccb->data->getDataSize());
          return ret < 0 ? BLE_ATT_ERR_UNLIKELY : 0;
        }

      case BLE_GATT_ACCESS_OP_WRITE_CHR:
        {
          uint16_t len = 0;

          ret =
            ble_hs_mbuf_to_flat(ctxt->om, ccb->data->getDataPtr(), ccb->data->getDataSize(), &len);
          if (ret < 0)
            {
              return BLE_ATT_ERR_UNLIKELY;
            }

          ret = ccb->io->setData(*ccb->data);
          return ret < 0 ? BLE_ATT_ERR_UNLIKELY : 0;
        }

      default:
        return BLE_ATT_ERR_UNLIKELY;
    }
}

CProtoNimblePrphCustom::CProtoNimblePrphCustom(const SObjectCfg::SObjectCfgItem *item,
                                               IProtoNimblePrphCb *cb_)
  : IProtoNimblePrphService(item, cb_)
  , id(-1)
  , created(false)
  , chrs(nullptr)
{
  std::memset(&svc, 0, sizeof(svc));
}

CProtoNimblePrphCustom::~CProtoNimblePrphCustom()
{
  deinit();
}

void CProtoNimblePrphCustom::configureDesc(const SObjectCfg::SObjectCfgItem *item)
{
  const SProtoNimblePrphCustomSvc *blob;
  uint32_t count;

  blob = reinterpret_cast<const SProtoNimblePrphCustomSvc *>(item->data);
  count = blob->cfg0;

  svc.type = BLE_GATT_SVC_TYPE_PRIMARY;
  svc.includes = nullptr;
  svc.uuid = nullptr;

  for (uint32_t i = 0; i < count; i++)
    {
      SCustomChar entry{};
      entry.objid = blob->chr[i].objid.v;
      entry.flags = static_cast<uint8_t>(blob->chr[i].flags & 0xff);
      std::memcpy(entry.uuid, blob->chr[i].uuid, sizeof(entry.uuid));
      vchars.push_back(entry);
      cb->regObject(entry.objid);
      vio.push_back(entry.objid);
    }
}

int CProtoNimblePrphCustom::allocCustom()
{
  const SProtoNimblePrphCustomSvc *blob;

  blob = reinterpret_cast<const SProtoNimblePrphCustomSvc *>(desc->data);

  chrs = new (std::nothrow) ble_gatt_chr_def[vchars.size() + 1]();
  if (chrs == nullptr)
    {
      return -ENOMEM;
    }

  ble_uuid128_t *svc_uuid = new (std::nothrow) ble_uuid128_t{};
  if (svc_uuid == nullptr)
    {
      delete[] chrs;
      chrs = nullptr;
      return -ENOMEM;
    }

  svc_uuid->u.type = BLE_UUID_TYPE_128;
  std::memcpy(svc_uuid->value, blob->uuid, sizeof(svc_uuid->value));
  svc.uuid = &svc_uuid->u;
  svc.characteristics = chrs;
  return OK;
}

int CProtoNimblePrphCustom::createCustom()
{
  for (size_t i = 0; i < vchars.size(); i++)
    {
      CIOCommon *io = cb->getObject(vchars[i].objid);
      SCustomCharCb *ccb;
      uint8_t effective = 0;

      if (io == nullptr)
        {
          return -EIO;
        }

      if (vchars[i].flags & CUSTOM_CHR_F_READ && io->isRead())
        {
          effective |= BLE_GATT_CHR_F_READ;
        }

      if (vchars[i].flags & CUSTOM_CHR_F_WRITE && io->isWrite())
        {
          effective |= BLE_GATT_CHR_F_WRITE;
        }

#ifdef CONFIG_DAWN_IO_NOTIFY
      if (vchars[i].flags & CUSTOM_CHR_F_NOTIFY && io->isNotify())
        {
          effective |= BLE_GATT_CHR_F_NOTIFY;
        }
#endif

      if (effective == 0)
        {
          return -EINVAL;
        }

      ble_uuid128_t *chr_uuid = new (std::nothrow) ble_uuid128_t{};
      ccb = new (std::nothrow) SCustomCharCb{};
      if (chr_uuid == nullptr || ccb == nullptr)
        {
          delete chr_uuid;
          delete ccb;
          return -ENOMEM;
        }

      chr_uuid->u.type = BLE_UUID_TYPE_128;
      std::memcpy(chr_uuid->value, vchars[i].uuid, sizeof(chr_uuid->value));

      ccb->io = io;
      ccb->handle = 0;
      ccb->data = io->ddata_alloc(1);
      if (ccb->data == nullptr)
        {
          delete chr_uuid;
          delete ccb;
          return -ENOMEM;
        }

#ifdef CONFIG_DAWN_IO_NOTIFY
      if (effective & BLE_GATT_CHR_F_NOTIFY)
        {
          int ret = io->setNotifier(notifierCb, 0, ccb);
          if (ret < 0)
            {
              delete ccb->data;
              delete chr_uuid;
              delete ccb;
              return ret;
            }
        }
#endif

      vcbs.push_back(ccb);

      chrs[i].uuid = &chr_uuid->u;
      chrs[i].access_cb = callback;
      chrs[i].arg = ccb;
      chrs[i].descriptors = nullptr;
      chrs[i].flags = effective;
      chrs[i].min_key_size = 0;
      chrs[i].val_handle = (effective & BLE_GATT_CHR_F_NOTIFY) ? &ccb->handle : nullptr;
    }

  return OK;
}

void CProtoNimblePrphCustom::deleteCustom()
{
  for (auto *ccb : vcbs)
    {
      if (ccb != nullptr)
        {
#ifdef CONFIG_DAWN_IO_NOTIFY
          if (ccb->io != nullptr && ccb->io->isNotify())
            {
              ccb->io->setNotifier(nullptr, 0, nullptr);
            }
#endif

          if (ccb->data != nullptr)
            {
              delete ccb->data;
            }
          delete ccb;
        }
    }
  vcbs.clear();

  if (chrs != nullptr)
    {
      for (size_t i = 0; i < vchars.size(); i++)
        {
          if (chrs[i].uuid != nullptr)
            {
              delete reinterpret_cast<const ble_uuid128_t *>(chrs[i].uuid);
            }
        }

      delete[] chrs;
      chrs = nullptr;
    }

  if (svc.uuid != nullptr)
    {
      delete reinterpret_cast<const ble_uuid128_t *>(svc.uuid);
      svc.uuid = nullptr;
    }

  svc.characteristics = nullptr;
  created = false;
}

int CProtoNimblePrphCustom::init()
{
  int ret;

  configureDesc(desc);
  ret = allocCustom();
  if (ret != OK)
    {
      return ret;
    }

  id = cb->serviceRegister(&svc);
  return id < 0 ? -EIO : OK;
}

int CProtoNimblePrphCustom::deinit()
{
  deleteCustom();
  return OK;
}

int CProtoNimblePrphCustom::start()
{
  int ret;

  if (created)
    {
      return cb->startService(id);
    }

  ret = createCustom();
  if (ret != OK)
    {
      deleteCustom();
      allocCustom();
      return ret;
    }

  created = true;
  return cb->startService(id);
}

int CProtoNimblePrphCustom::stop()
{
  return cb->stopService(id);
}
