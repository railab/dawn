// dawn/src/proto/nimble/prph_aios.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/prph_aios.hxx"

#include <cstdlib>
#include <cstring>
#include <new>

#include "dawn/io/common.hxx"
#include "dawn/proto/nimble/gatt_runtime.hxx"
#include "host/ble_hs_mbuf.h"
#include "os/os_mbuf.h"

using namespace dawn;

#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
static ble_uuid_any_t *aiosMakeUuid16(uint16_t value)
{
  ble_uuid_any_t *uuid = new (std::nothrow) ble_uuid_any_t{};
  if (uuid == nullptr)
    {
      return nullptr;
    }

  uuid->u.type = BLE_UUID_TYPE_16;
  uuid->u16.value = value;
  return uuid;
}

static void aiosFreeUuid(const ble_uuid_t *uuid)
{
  if (uuid != nullptr)
    {
      delete reinterpret_cast<const ble_uuid_any_t *>(uuid);
    }
}

static int aiosReadFieldRaw(CIOCommon *io, io_ddata_t *data, uint8_t *value, uint8_t *len)
{
  int ret;

  if (io == nullptr)
    {
      *len = 0;
      return OK;
    }

  if (io->getDataSize() > CProtoNimblePrphAios::AIOS_RAW_DESCRIPTOR_MAX)
    {
      return -EOVERFLOW;
    }

  ret = io->getData(*data, io->getDataDim());
  if (ret < 0)
    {
      return ret;
    }

  *len = static_cast<uint8_t>(io->getDataSize());
  std::memcpy(value, data->getDataPtr(), *len);
  return OK;
}
#endif

#ifdef CONFIG_DAWN_IO_NOTIFY
int CProtoNimblePrphAios::notifierCb(void *priv, io_ddata_t *data)
{
  SPrphNotiferCb *ncb = static_cast<SPrphNotiferCb *>(priv);

  if (ncb == nullptr)
    {
      DAWNERR("NULL ncb pointer\n");
      return -EINVAL;
    }

  CIOCommon *io = reinterpret_cast<CIOCommon *>(ncb->io);

  if (io == nullptr)
    {
      DAWNERR("NULL io pointer\n");
      return -EINVAL;
    }

  // Copy data to local storage and check if value changed

  std::memcpy(ncb->data->getDataPtr(), data->getDataPtr(), ncb->data->getDataSize());

  // Notify nimble

  ble_gatts_chr_updated(ncb->handle);

  return OK;
}
#endif

#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
int CProtoNimblePrphAios::descriptorCb(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg)
{
  SAiosDscCb *dcb = static_cast<SAiosDscCb *>(arg);
  int ret;

  (void)conn_handle;
  (void)attr_handle;

  if (dcb == nullptr || ctxt->op != BLE_GATT_ACCESS_OP_READ_DSC)
    {
      return BLE_ATT_ERR_UNLIKELY;
    }

#  if defined(CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_VALUE_TRIGGER_SETTING) || \
    defined(CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_TIME_TRIGGER_SETTING)
  if (dcb->kind == AIOS_DESC_VALUE_TRIGGER_SETTING || dcb->kind == AIOS_DESC_TIME_TRIGGER_SETTING)
    {
      ret = aiosReadFieldRaw(dcb->field.io, dcb->field.data, dcb->data, &dcb->len);
      if (ret < 0)
        {
          DAWNERR("AIOS raw descriptor field read failed %d\n", ret);
          return BLE_ATT_ERR_UNLIKELY;
        }
    }
#  endif

  ret = os_mbuf_append(ctxt->om, dcb->data, dcb->len);
  if (ret < 0)
    {
      DAWNERR("AIOS descriptor append failed %d\n", ret);
      return BLE_ATT_ERR_UNLIKELY;
    }

  return 0;
}
#endif

// NOTE: For analog IO - 4B data are supported

int CProtoNimblePrphAios::callbackAnalog(uint16_t conn_handle,
                                         uint16_t attr_handle,
                                         struct ble_gatt_access_ctxt *ctxt,
                                         void *arg)
{
  SPrphNotiferCb *ncb = static_cast<SPrphNotiferCb *>(arg);
  CIOCommon *io = reinterpret_cast<CIOCommon *>(ncb->io);
  uint16_t uuid16 = ble_uuid_u16(ctxt->chr->uuid);
  int ret;

  if (uuid16 != IProtoNimblePrphCb::UUID16_ANALOG)
    {
      DAWNERR("AIOS analog UUID mismatch\n");
      return BLE_ATT_ERR_UNLIKELY;
    }

  switch (ctxt->op)
    {
      case BLE_GATT_ACCESS_OP_READ_CHR:
        {
          size_t size = std::min(ncb->data->getDataSize(), sizeof(uint32_t));
          uint32_t retval = 0;

#ifdef CONFIG_DAWN_IO_NOTIFY
          if (!io->isNotify())
#endif
            {
              // Get data

              ret = io->getData(*ncb->data, 1);
              if (ret < 0)
                {
                  return BLE_ATT_ERR_UNLIKELY;
                }
            }

          // Copy data

          std::memcpy(&retval, &ncb->data->get<uint32_t>(0), size);

          // Write data

          ret = os_mbuf_append(ctxt->om, &retval, sizeof(uint32_t));
          if (ret < 0)
            {
              DAWNERR("os_mbuf_append failed %d\n", ret);
            }

          return 0;
        }

      case BLE_GATT_ACCESS_OP_WRITE_CHR:
        {
          size_t size = std::min(ncb->data->getDataSize(), sizeof(uint32_t));
          uint16_t len = 0;
          uint32_t retval = 0;

          // Get data

          ret = ble_hs_mbuf_to_flat(ctxt->om, &retval, sizeof(uint32_t), &len);
          if (ret < 0)
            {
              return BLE_ATT_ERR_UNLIKELY;
            }

          // Copy data

          std::memcpy(&ncb->data->get<uint32_t>(0), &retval, size);

          // Write state

          ret = io->setData(*ncb->data);
          if (ret < 0)
            {
              return BLE_ATT_ERR_UNLIKELY;
            }

          return 0;
        }

      default:
        {
          DAWNERR("AIOS GATT operation not supported\n");
          return BLE_ATT_ERR_UNLIKELY;
          return BLE_ATT_ERR_UNLIKELY;
        }
    }

  return BLE_ATT_ERR_UNLIKELY;
}

// NOTE: For digial IO - 1B data are supported
int CProtoNimblePrphAios::callbackDigital(uint16_t conn_handle,
                                          uint16_t attr_handle,
                                          struct ble_gatt_access_ctxt *ctxt,
                                          void *arg)
{
  SPrphNotiferCb *ncb = static_cast<SPrphNotiferCb *>(arg);
  CIOCommon *io = reinterpret_cast<CIOCommon *>(ncb->io);
  uint16_t uuid16 = ble_uuid_u16(ctxt->chr->uuid);
  int ret;

  if (uuid16 != IProtoNimblePrphCb::UUID16_DIGITAL)
    {
      DAWNERR("AIOS digital UUID mismatch\n");
      return BLE_ATT_ERR_UNLIKELY;
    }

  switch (ctxt->op)
    {
      case BLE_GATT_ACCESS_OP_READ_CHR:
        {
#ifdef CONFIG_DAWN_IO_NOTIFY
          if (!io->isNotify())
#endif
            {
              // Get data

              ret = io->getData(*ncb->data, 1);
              if (ret < 0)
                {
                  return BLE_ATT_ERR_UNLIKELY;
                }
            }

          // Write data

          ret = os_mbuf_append(ctxt->om, &ncb->data->get<bool>(0), sizeof(bool));
          if (ret < 0)
            {
              DAWNERR("os_mbuf_append failed %d\n", ret);
            }

          return 0;
        }

      case BLE_GATT_ACCESS_OP_WRITE_CHR:
        {
          uint16_t len;

          // Get data

          ret = ble_hs_mbuf_to_flat(ctxt->om, &ncb->data->get<bool>(0), sizeof(bool), &len);
          if (ret < 0)
            {
              return BLE_ATT_ERR_UNLIKELY;
            }

          // Write state

          ret = io->setData(*ncb->data);
          if (ret < 0)
            {
              return BLE_ATT_ERR_UNLIKELY;
            }

          return 0;
        }

      default:
        {
          DAWNERR("AIOS GATT operation not supported\n");
          return BLE_ATT_ERR_UNLIKELY;
          return BLE_ATT_ERR_UNLIKELY;
        }
    }

  return BLE_ATT_ERR_UNLIKELY;
}

#ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_AGGREGATE
int CProtoNimblePrphAios::callbackAggregate(uint16_t conn_handle,
                                            uint16_t attr_handle,
                                            struct ble_gatt_access_ctxt *ctxt,
                                            void *arg)
{
  CProtoNimblePrphAios *service = static_cast<CProtoNimblePrphAios *>(arg);
  uint16_t uuid16 = ble_uuid_u16(ctxt->chr->uuid);

  UNUSED(conn_handle);
  UNUSED(attr_handle);

  if (service == nullptr || ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR ||
      uuid16 != IProtoNimblePrphCb::UUID16_AGGREGATE)
    {
      return BLE_ATT_ERR_UNLIKELY;
    }

  return service->appendAggregateValue(ctxt->om);
}

int CProtoNimblePrphAios::appendAggregateValue(struct os_mbuf *om)
{
  int ret;

  for (auto const objid : vio)
    {
      CIOCommon *io = cb->getObject(objid);
      SPrphNotiferCb *ncb = ioCbMap[objid];
      uint8_t type = ioTypeMap[objid];

      if (io == nullptr || ncb == nullptr || ncb->data == nullptr || !io->isRead())
        {
          continue;
        }

#  ifdef CONFIG_DAWN_IO_NOTIFY
      if (!io->isNotify())
#  endif
        {
          ret = io->getData(*ncb->data, 1);
          if (ret < 0)
            {
              return BLE_ATT_ERR_UNLIKELY;
            }
        }

      if (type == PRPH_AIOS_TYPE_DIGITAL)
        {
          bool value = false;
          std::memcpy(&value, ncb->data->getDataPtr(), sizeof(value));
          ret = os_mbuf_append(om, &value, sizeof(value));
          if (ret < 0)
            {
              DAWNERR("AIOS aggregate digital append failed %d\n", ret);
              return BLE_ATT_ERR_UNLIKELY;
            }
        }
      else if (type == PRPH_AIOS_TYPE_ANALOG)
        {
          size_t size = std::min(ncb->data->getDataSize(), sizeof(uint32_t));
          uint32_t value = 0;

          std::memcpy(&value, ncb->data->getDataPtr(), size);
          ret = os_mbuf_append(om, &value, sizeof(value));
          if (ret < 0)
            {
              DAWNERR("AIOS aggregate analog append failed %d\n", ret);
              return BLE_ATT_ERR_UNLIKELY;
            }
        }
    }

  return 0;
}
#endif

int CProtoNimblePrphAios::allocAIOS()
{
  // Configure service

  svc.type = BLE_GATT_SVC_TYPE_PRIMARY;
  svc.uuid = reinterpret_cast<const ble_uuid_t *>(IProtoNimblePrphCb::UUID_AIOS);
  svc.includes = nullptr;

  // One more characteristic for end of array

  noChar = cb->getObjectsLen() + 1;
#ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_AGGREGATE
  if (aggregateEnabled)
    {
      noChar++;
    }
#endif

  // Allocate memory for characterisitcs

  svc.characteristics = new (std::nothrow) ble_gatt_chr_def[noChar]();
  if (svc.characteristics == nullptr)
    {
      DAWNERR("Failed to allocate characteristics\n");
      return -ENOMEM;
    }

  return OK;
}

int CProtoNimblePrphAios::createAIOS()
{
  const uint32_t *char_uuid = nullptr;
  int ret;
  int j = 0;

  if (svc.characteristics == nullptr)
    {
      DAWNERR("Failed to allocate characteristics\n");
      return -ENOMEM;
    }

  // Configure service

  svc.type = BLE_GATT_SVC_TYPE_PRIMARY;
  svc.uuid = reinterpret_cast<const ble_uuid_t *>(IProtoNimblePrphCb::UUID_AIOS);
  svc.includes = nullptr;

  for (auto const objid : vio)
    {
      CIOCommon *io = cb->getObject(objid);

      // NOTE: NimBLE C API declares svc.characteristics as
      // const ble_gatt_chr_def*, but Dawn allocates it dynamically
      // and needs write access to populate fields at runtime.

      struct ble_gatt_chr_def *chr = const_cast<struct ble_gatt_chr_def *>(&svc.characteristics[j]);
      SPrphNotiferCb *ncb;

      if (io == nullptr)
        {
          DAWNERR("AIOS IO not found\n");
          return -EIO;
        }

      // Seekable IOs not supported by AIOS

      if (io->isSeekable())
        {
          DAWNERR("seekable IO not supported by AIOS"
                  " (objid=0x%08" PRIx32 ")\n",
                  io->getIdV());
          return -ENOTSUP;
        }

      ncb = nimbleGattNotifierCreate(io);
      if (ncb == nullptr)
        {
          DAWNERR("AIOS notifier allocation failed\n");
          return -ENOMEM;
        }

      // Connect callback data

      ioCbMap.insert_or_assign(objid, ncb);

      // Fill characteristic, access_cb is fill later

      nimbleGattChrInit(*chr, ncb);
      char_uuid = nullptr;

      // Check characteristic properties

#ifdef CONFIG_DAWN_IO_NOTIFY
      ret = nimbleGattChrNotifySet(*chr, *io, *ncb, notifierCb, objid);
      if (ret < 0)
        {
          DAWNERR("AIOS notifier setup failed\n");
        }
#endif

      nimbleGattChrAccessSet(*chr, *io);

      // Get IO class

      switch (ioTypeMap[objid])
        {
          case PRPH_AIOS_TYPE_DIGITAL:
            {
              chr->access_cb = CProtoNimblePrphAios::callbackDigital;
              char_uuid = IProtoNimblePrphCb::UUID_DIGITAL;
              break;
            }

          case PRPH_AIOS_TYPE_ANALOG:
            {
              chr->access_cb = CProtoNimblePrphAios::callbackAnalog;
              char_uuid = IProtoNimblePrphCb::UUID_ANALOG;
              break;
            }

          default:
            {
              DAWNERR("unknown AIOS type %d\n", ioTypeMap[objid]);
              return -EINVAL;
            }
        }

      // Assignsome dummy value

      if (char_uuid == nullptr)
        {
          DAWNERR("No UUID for AIOS IO\n");
          return -EINVAL;
        }

      ret = nimbleGattChrUuid128Set(*chr, char_uuid);
      if (ret != OK)
        {
          DAWNERR("AIOS UUID allocation failed\n");
          return ret;
        }

      ret = configureDescriptors(chr, objid);
      if (ret != OK)
        {
          return ret;
        }

      // Next service

      j++;
    }

#ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_AGGREGATE
  if (aggregateEnabled)
    {
      struct ble_gatt_chr_def *chr = const_cast<struct ble_gatt_chr_def *>(&svc.characteristics[j]);

      nimbleGattChrInit(*chr, this);
      ret = nimbleGattChrUuid128Set(*chr, IProtoNimblePrphCb::UUID_AGGREGATE);
      if (ret != OK)
        {
          DAWNERR("AIOS aggregate UUID allocation failed\n");
          return ret;
        }

      chr->access_cb = CProtoNimblePrphAios::callbackAggregate;
      chr->flags = BLE_GATT_CHR_F_READ;
      j++;
    }
#endif

  return OK;
}

void CProtoNimblePrphAios::deleteAIOS()
{
  // NOTE: svc.characteristics fields are const-qualified per NimBLE API.
  // Dawn allocated this memory and is the rightful owner, so the cast is safe.

  if (svc.characteristics)
    {
      for (size_t i = 0; i < noChar - 1; i++)
        {
          if (svc.characteristics[i].uuid)
            {
              if (ble_uuid_u16(svc.characteristics[i].uuid) == IProtoNimblePrphCb::UUID16_AGGREGATE)
                {
                  nimbleGattChrUuidFree(
                    const_cast<struct ble_gatt_chr_def &>(svc.characteristics[i]));
                  continue;
                }

              nimbleGattChrUuidFree(const_cast<struct ble_gatt_chr_def &>(svc.characteristics[i]));
            }

#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
          if (svc.characteristics[i].descriptors)
            {
              struct ble_gatt_dsc_def *dsc =
                const_cast<struct ble_gatt_dsc_def *>(svc.characteristics[i].descriptors);

              for (size_t j = 0; dsc[j].uuid != nullptr; j++)
                {
                  SAiosDscCb *dcb = static_cast<SAiosDscCb *>(dsc[j].arg);

                  aiosFreeUuid(dsc[j].uuid);
                  if (dcb != nullptr)
                    {
                      delete dcb->field.data;
                      delete dcb;
                    }
                }

              delete[] dsc;
            }
#endif

          nimbleGattChrNotifierFree(const_cast<struct ble_gatt_chr_def &>(svc.characteristics[i]));
        }

      delete[] const_cast<ble_gatt_chr_def *>(svc.characteristics);

      svc.characteristics = nullptr;
    }

  ioCbMap.clear();
  created = false;
}

int CProtoNimblePrphAios::configureDescriptors(struct ble_gatt_chr_def *chr,
                                               SObjectId::ObjectId objid)
{
#ifndef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
  UNUSED(chr);
  UNUSED(objid);
  return OK;
#else
  const auto metaIt = ioMetaMap.find(objid);
  SAiosMeta *meta;
  struct ble_gatt_dsc_def *dsc;
  size_t descCount;
  size_t idx;

  if (metaIt == ioMetaMap.end())
    {
      return OK;
    }

  meta = const_cast<SAiosMeta *>(&metaIt->second);
  descCount = 0;
#  ifndef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_USER_DESCRIPTION
  meta->desc &= ~AIOS_DESC_USER_DESCRIPTION;
#  endif
  if (meta->desc & AIOS_DESC_USER_DESCRIPTION)
    {
      descCount++;
    }

#  ifndef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_NUMBER_OF_DIGITALS
  meta->desc &= ~AIOS_DESC_NUMBER_OF_DIGITALS;
#  endif
  if (meta->desc & AIOS_DESC_NUMBER_OF_DIGITALS)
    {
      descCount++;
    }

#  ifndef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_VALUE_TRIGGER_SETTING
  meta->desc &= ~AIOS_DESC_VALUE_TRIGGER_SETTING;
#  endif
  if (meta->desc & AIOS_DESC_VALUE_TRIGGER_SETTING)
    {
      descCount++;
    }

#  ifndef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_TIME_TRIGGER_SETTING
  meta->desc &= ~AIOS_DESC_TIME_TRIGGER_SETTING;
#  endif
  if (meta->desc & AIOS_DESC_TIME_TRIGGER_SETTING)
    {
      descCount++;
    }

#  ifndef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_PRESENTATION_FORMAT
  meta->desc &= ~AIOS_DESC_PRESENTATION_FORMAT;
#  endif
  if (meta->desc & AIOS_DESC_PRESENTATION_FORMAT)
    {
      descCount++;
    }

#  ifndef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_EXTENDED_PROPERTIES
  meta->desc &= ~AIOS_DESC_EXTENDED_PROPERTIES;
#  endif
  if (meta->desc & AIOS_DESC_EXTENDED_PROPERTIES)
    {
      descCount++;
    }

  if (descCount == 0)
    {
      return OK;
    }

  dsc = new (std::nothrow) ble_gatt_dsc_def[descCount + 1]();
  if (dsc == nullptr)
    {
      return -ENOMEM;
    }

  idx = 0;
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_USER_DESCRIPTION
  if (meta->desc & AIOS_DESC_USER_DESCRIPTION)
    {
      SAiosDscCb *dcb = new (std::nothrow) SAiosDscCb{};
      ble_uuid_any_t *uuid = aiosMakeUuid16(UUID16_CHR_USER_DESCRIPTION);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          aiosFreeUuid(uuid ? &uuid->u : nullptr);
          goto fail;
        }

      dcb->kind = AIOS_DESC_USER_DESCRIPTION;
      dcb->len = 0;
      while (dcb->len < AIOS_USER_DESCRIPTION_MAX && meta->userDescription[dcb->len] != '\0')
        {
          dcb->data[dcb->len] = static_cast<uint8_t>(meta->userDescription[dcb->len]);
          dcb->len++;
        }

      dsc[idx].uuid = &uuid->u;
      dsc[idx].att_flags = BLE_ATT_F_READ;
      dsc[idx].access_cb = descriptorCb;
      dsc[idx].arg = dcb;
      idx++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_NUMBER_OF_DIGITALS
  if (meta->desc & AIOS_DESC_NUMBER_OF_DIGITALS)
    {
      SAiosDscCb *dcb = new (std::nothrow) SAiosDscCb{};
      ble_uuid_any_t *uuid = aiosMakeUuid16(UUID16_NUMBER_OF_DIGITALS);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          aiosFreeUuid(uuid ? &uuid->u : nullptr);
          goto fail;
        }

      dcb->kind = AIOS_DESC_NUMBER_OF_DIGITALS;
      dcb->len = 1;
      dcb->data[0] = meta->numberOfDigitals;

      dsc[idx].uuid = &uuid->u;
      dsc[idx].att_flags = BLE_ATT_F_READ;
      dsc[idx].access_cb = descriptorCb;
      dsc[idx].arg = dcb;
      idx++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_EXTENDED_PROPERTIES
  if (meta->desc & AIOS_DESC_EXTENDED_PROPERTIES)
    {
      SAiosDscCb *dcb = new (std::nothrow) SAiosDscCb{};
      ble_uuid_any_t *uuid = aiosMakeUuid16(UUID16_EXTENDED_PROPERTIES);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          aiosFreeUuid(uuid ? &uuid->u : nullptr);
          goto fail;
        }

      dcb->kind = AIOS_DESC_EXTENDED_PROPERTIES;
      dcb->len = AIOS_EXTENDED_PROPERTIES_SIZE;
      dcb->data[0] = static_cast<uint8_t>(meta->extendedProperties & 0xff);
      dcb->data[1] = static_cast<uint8_t>((meta->extendedProperties >> 8) & 0xff);

      dsc[idx].uuid = &uuid->u;
      dsc[idx].att_flags = BLE_ATT_F_READ;
      dsc[idx].access_cb = descriptorCb;
      dsc[idx].arg = dcb;
      idx++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_PRESENTATION_FORMAT
  if (meta->desc & AIOS_DESC_PRESENTATION_FORMAT)
    {
      SAiosDscCb *dcb = new (std::nothrow) SAiosDscCb{};
      ble_uuid_any_t *uuid = aiosMakeUuid16(UUID16_PRESENTATION_FORMAT);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          aiosFreeUuid(uuid ? &uuid->u : nullptr);
          goto fail;
        }

      dcb->kind = AIOS_DESC_PRESENTATION_FORMAT;
      dcb->len = AIOS_PRESENTATION_FORMAT_SIZE;
      std::memcpy(dcb->data, meta->presentationFormat, dcb->len);

      dsc[idx].uuid = &uuid->u;
      dsc[idx].att_flags = BLE_ATT_F_READ;
      dsc[idx].access_cb = descriptorCb;
      dsc[idx].arg = dcb;
      idx++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_VALUE_TRIGGER_SETTING
  if (meta->desc & AIOS_DESC_VALUE_TRIGGER_SETTING)
    {
      SAiosDscCb *dcb = new (std::nothrow) SAiosDscCb{};
      ble_uuid_any_t *uuid = aiosMakeUuid16(UUID16_VALUE_TRIGGER_SETTING);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          aiosFreeUuid(uuid ? &uuid->u : nullptr);
          goto fail;
        }

      dcb->kind = AIOS_DESC_VALUE_TRIGGER_SETTING;
      dcb->field.io = cb->getObject(meta->valueTriggerSettingObjid);
      if (dcb->field.io == nullptr)
        {
          delete dcb;
          aiosFreeUuid(&uuid->u);
          goto fail;
        }

      dcb->field.data = dcb->field.io->ddata_alloc(1);
      if (dcb->field.data == nullptr)
        {
          delete dcb;
          aiosFreeUuid(&uuid->u);
          goto fail;
        }

      dsc[idx].uuid = &uuid->u;
      dsc[idx].att_flags = BLE_ATT_F_READ;
      dsc[idx].access_cb = descriptorCb;
      dsc[idx].arg = dcb;
      idx++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_TIME_TRIGGER_SETTING
  if (meta->desc & AIOS_DESC_TIME_TRIGGER_SETTING)
    {
      SAiosDscCb *dcb = new (std::nothrow) SAiosDscCb{};
      ble_uuid_any_t *uuid = aiosMakeUuid16(UUID16_TIME_TRIGGER_SETTING);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          aiosFreeUuid(uuid ? &uuid->u : nullptr);
          goto fail;
        }

      dcb->kind = AIOS_DESC_TIME_TRIGGER_SETTING;
      dcb->field.io = cb->getObject(meta->timeTriggerSettingObjid);
      if (dcb->field.io == nullptr)
        {
          delete dcb;
          aiosFreeUuid(&uuid->u);
          goto fail;
        }

      dcb->field.data = dcb->field.io->ddata_alloc(1);
      if (dcb->field.data == nullptr)
        {
          delete dcb;
          aiosFreeUuid(&uuid->u);
          goto fail;
        }

      dsc[idx].uuid = &uuid->u;
      dsc[idx].att_flags = BLE_ATT_F_READ;
      dsc[idx].access_cb = descriptorCb;
      dsc[idx].arg = dcb;
      idx++;
    }
#  endif

  chr->descriptors = dsc;
  return OK;

fail:
  for (size_t i = 0; i < idx; i++)
    {
      aiosFreeUuid(dsc[i].uuid);
      SAiosDscCb *dcb = static_cast<SAiosDscCb *>(dsc[i].arg);
      if (dcb != nullptr)
        {
          delete dcb->field.data;
          delete dcb;
        }
    }

  delete[] dsc;
  return -ENOMEM;
#endif
}

void CProtoNimblePrphAios::allocObject(const SProtoNimblePrphIOBindAiosObjid &obj,
                                       const uint32_t *ext)
{
  uint8_t type = cfgIdIOBindAiosCfgObjTypeGet(obj.cfg);
#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
  SAiosMeta meta{};
#endif

  DAWNINFO("AIOS allocate type=%d object 0x%" PRIx32 "\n", type, obj.objid.v);

  if (type != PRPH_AIOS_TYPE_DIGITAL && type != PRPH_AIOS_TYPE_ANALOG)
    {
      DAWNERR("Invalid AIOS type %d for object 0x%" PRIx32 "\n", type, obj.objid.v);
      return;
    }

  // Store type for later

  ioTypeMap.insert_or_assign(obj.objid.v, type);

#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
  for (uint32_t i = 0; i < obj.extCount; i++)
    {
      uint8_t kind = cfgIdIOBindAiosExtKindGet(ext[0]);
      uint8_t size = cfgIdIOBindAiosExtSizeGet(ext[0]);
      const uint32_t *data = &ext[1];

      switch (kind)
        {
          case AIOS_EXT_USER_DESCRIPTION:
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_USER_DESCRIPTION
            if (size == AIOS_USER_DESCRIPTION_MAX / sizeof(uint32_t))
              {
                meta.desc |= AIOS_DESC_USER_DESCRIPTION;
                std::memcpy(meta.userDescription, data, sizeof(meta.userDescription));
              }
#  endif
            break;

          case AIOS_EXT_NUMBER_OF_DIGITALS:
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_NUMBER_OF_DIGITALS
            if (size == 1)
              {
                meta.desc |= AIOS_DESC_NUMBER_OF_DIGITALS;
                meta.numberOfDigitals = static_cast<uint8_t>(data[0]);
              }
#  endif
            break;

          case AIOS_EXT_VALUE_TRIGGER_SETTING:
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_VALUE_TRIGGER_SETTING
            if (size == 1)
              {
                meta.desc |= AIOS_DESC_VALUE_TRIGGER_SETTING;
                meta.valueTriggerSettingObjid = data[0];
              }
#  endif
            break;

          case AIOS_EXT_TIME_TRIGGER_SETTING:
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_TIME_TRIGGER_SETTING
            if (size == 1)
              {
                meta.desc |= AIOS_DESC_TIME_TRIGGER_SETTING;
                meta.timeTriggerSettingObjid = data[0];
              }
#  endif
            break;

          case AIOS_EXT_PRESENTATION_FORMAT:
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_PRESENTATION_FORMAT
            if (size == 2)
              {
                meta.desc |= AIOS_DESC_PRESENTATION_FORMAT;
                meta.presentationFormat[0] = static_cast<uint8_t>(data[0] & 0xff);
                meta.presentationFormat[1] = static_cast<uint8_t>((data[0] >> 8) & 0xff);
                meta.presentationFormat[2] = static_cast<uint8_t>((data[0] >> 16) & 0xff);
                meta.presentationFormat[3] = static_cast<uint8_t>((data[0] >> 24) & 0xff);
                meta.presentationFormat[4] = static_cast<uint8_t>(data[1] & 0xff);
                meta.presentationFormat[5] = static_cast<uint8_t>((data[1] >> 8) & 0xff);
                meta.presentationFormat[6] = static_cast<uint8_t>((data[1] >> 16) & 0xff);
              }
#  endif
            break;

          case AIOS_EXT_EXTENDED_PROPERTIES:
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_EXTENDED_PROPERTIES
            if (size == 1)
              {
                meta.desc |= AIOS_DESC_EXTENDED_PROPERTIES;
                meta.extendedProperties = static_cast<uint16_t>(data[0] & 0xffff);
              }
#  endif
            break;

          default:
            break;
        }

      ext += 1 + size;
    }

  if (meta.desc != 0)
    {
      ioMetaMap.insert_or_assign(obj.objid.v, meta);
    }
#endif

  // Allocate object in map

  cb->regObject(obj.objid.v);
#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_VALUE_TRIGGER_SETTING
  if (meta.valueTriggerSettingObjid != 0)
    {
      cb->regObject(meta.valueTriggerSettingObjid);
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_TIME_TRIGGER_SETTING
  if (meta.timeTriggerSettingObjid != 0)
    {
      cb->regObject(meta.timeTriggerSettingObjid);
    }
#  endif
#endif

  vio.push_back(obj.objid.v);
}

// Assumption: Configuration item passed by caller is a valid type supported by
// this class.
void CProtoNimblePrphAios::configureDesc(const SObjectCfg::SObjectCfgItem *item)
{
  // Iterrate over objectid list

  for (size_t k = 0; k < item->cfgid.s.size;)
    {
      const SProtoNimblePrphIOBindAios *tmp =
        reinterpret_cast<const SProtoNimblePrphIOBindAios *>(&item->data[k]);

      uint8_t size = cfgIdIOBindAiosCfg0SizeGet(tmp->cfg0);
#ifdef CONFIG_DAWN_PROTO_NIMBLE_AIOS_AGGREGATE
      aggregateEnabled = aggregateEnabled || cfgIdIOBindAiosCfg1AggregateGet(tmp->cfg1);
#endif

      size_t pos = k + sizeof(SProtoNimblePrphIOBindAios) / sizeof(uint32_t);
      for (size_t i = 0; i < size; i++)
        {
          const SProtoNimblePrphIOBindAiosObjid *obj =
            reinterpret_cast<const SProtoNimblePrphIOBindAiosObjid *>(&item->data[pos]);
          const uint32_t *ext =
            &item->data[pos + sizeof(SProtoNimblePrphIOBindAiosObjid) / sizeof(uint32_t)];

          // Allocate object

          allocObject(*obj, ext);

          pos += sizeof(SProtoNimblePrphIOBindAiosObjid) / sizeof(uint32_t);
          for (uint32_t e = 0; e < obj->extCount; e++)
            {
              pos += 1 + cfgIdIOBindAiosExtSizeGet(item->data[pos]);
            }
        }

      k = pos;
    }
}

CProtoNimblePrphAios::CProtoNimblePrphAios(const SObjectCfg::SObjectCfgItem *desc_,
                                           IProtoNimblePrphCb *cb_)
  : IProtoNimblePrphService(desc_, cb_)
  , created(false)
{
  // Service ID not assigned yet

  id = -1;
  noChar = 0;
  aggregateEnabled = false;
}

CProtoNimblePrphAios::~CProtoNimblePrphAios()
{
  deinit();
}

int CProtoNimblePrphAios::init()
{
  int ret;

  // Configure object

  configureDesc(desc);

  // Allocate service

  ret = allocAIOS();
  if (ret != OK)
    {
      return ret;
    }

  // Register service in singleton

  id = cb->serviceRegister(&svc);
  if (id < 0)
    {
      DAWNERR("AIOS service registration failed\n");
      return -EIO;
    }

  return OK;
}

int CProtoNimblePrphAios::deinit()
{
  deleteAIOS();
  return OK;
}

int CProtoNimblePrphAios::start()
{
  int ret;

  if (created)
    {
      return cb->startService(id);
    }

  // Create service

  ret = createAIOS();
  if (ret != OK)
    {
      deleteAIOS();
      allocAIOS();
      return ret;
    }

  created = true;

  // Signal start to peripheral handler

  return cb->startService(id);
}

int CProtoNimblePrphAios::stop()
{
  // Signal stop to peripheral handler

  return cb->stopService(id);
}
