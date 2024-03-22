// dawn/src/proto/nimble/gatt_runtime.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/gatt_runtime.hxx"

#include <cerrno>
#include <cinttypes>
#include <cstring>
#include <new>

using namespace dawn;

void dawn::nimbleGattChrInit(struct ble_gatt_chr_def &chr, void *arg)
{
  chr.uuid = nullptr;
  chr.arg = arg;
  chr.descriptors = nullptr;
  chr.flags = 0;
  chr.min_key_size = 0;
  chr.val_handle = nullptr;
}

int dawn::nimbleGattChrUuid128Set(struct ble_gatt_chr_def &chr, const uint32_t *uuid)
{
  if (uuid == nullptr)
    {
      return -EINVAL;
    }

  chr.uuid = new (std::nothrow) ble_uuid_t[16]();
  if (chr.uuid == nullptr)
    {
      return -ENOMEM;
    }

  std::memcpy(const_cast<ble_uuid_t *>(chr.uuid), uuid, sizeof(uint8_t) * 16);
  return OK;
}

void dawn::nimbleGattChrUuidFree(struct ble_gatt_chr_def &chr)
{
  if (chr.uuid != nullptr)
    {
      delete[] const_cast<ble_uuid_t *>(chr.uuid);
      chr.uuid = nullptr;
    }
}

void dawn::nimbleGattChrAccessSet(struct ble_gatt_chr_def &chr, CIOCommon &io)
{
  if (io.isRead())
    {
      chr.flags |= BLE_GATT_CHR_F_READ;
    }

  if (io.isWrite())
    {
      chr.flags |= BLE_GATT_CHR_F_WRITE;
    }
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int dawn::nimbleGattChrNotifySet(struct ble_gatt_chr_def &chr,
                                 CIOCommon &io,
                                 IProtoNimblePrphService::SPrphNotiferCb &ncb,
                                 int (*notifier)(void *priv, io_ddata_t *data),
                                 SObjectId::ObjectId objid)
{
  int ret;

  if (!io.isNotify())
    {
      return OK;
    }

  ret = io.setNotifier(notifier, 0, &ncb);
  if (ret < 0)
    {
      DAWNERR("ERROR: set notifier failed for objId = 0x%" PRIx32 "\n", objid);
      return ret;
    }

  chr.flags |= BLE_GATT_CHR_F_NOTIFY;
  chr.val_handle = &ncb.handle;
  return OK;
}
#endif

IProtoNimblePrphService::SPrphNotiferCb *dawn::nimbleGattNotifierCreate(CIOCommon *io)
{
  IProtoNimblePrphService::SPrphNotiferCb *ncb;

  if (io == nullptr)
    {
      return nullptr;
    }

  ncb = new (std::nothrow) IProtoNimblePrphService::SPrphNotiferCb();
  if (ncb == nullptr)
    {
      return nullptr;
    }

  ncb->io = io;
  ncb->handle = 0;
  ncb->data = io->ddata_alloc(1);
  if (ncb->data == nullptr)
    {
      delete ncb;
      return nullptr;
    }

  return ncb;
}

void dawn::nimbleGattNotifierFree(IProtoNimblePrphService::SPrphNotiferCb *ncb)
{
  if (ncb == nullptr)
    {
      return;
    }

#ifdef CONFIG_DAWN_IO_NOTIFY
  if (ncb->io != nullptr && ncb->io->isNotify())
    {
      ncb->io->setNotifier(nullptr, 0, nullptr);
    }
#endif

  if (ncb->data != nullptr)
    {
      delete ncb->data;
    }
  delete ncb;
}

void dawn::nimbleGattChrNotifierFree(struct ble_gatt_chr_def &chr)
{
  nimbleGattNotifierFree(static_cast<IProtoNimblePrphService::SPrphNotiferCb *>(chr.arg));
  chr.arg = nullptr;
}
