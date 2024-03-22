// dawn/src/proto/nimble/prph_ess.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/prph_ess.hxx"

#include <cstdlib>
#include <new>

#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/proto/nimble/gatt_runtime.hxx"
#include "host/ble_hs_mbuf.h"
#include "os/os_mbuf.h"

using namespace dawn;

#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
#  if defined(CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_USER_DESCRIPTION) || \
    defined(CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_VALID_RANGE) ||        \
    defined(CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_MEASUREMENT) ||        \
    defined(CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_CONFIGURATION) ||      \
    defined(CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_TRIGGER_SETTING)
#    define DAWN_PROTO_NIMBLE_ESS_ANY_DESCRIPTOR 1
#  endif

#  ifdef DAWN_PROTO_NIMBLE_ESS_ANY_DESCRIPTOR
static ble_uuid_any_t *essMakeUuid16(uint16_t value)
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

static void essFreeUuid(const ble_uuid_t *uuid)
{
  if (uuid != nullptr)
    {
      delete reinterpret_cast<const ble_uuid_any_t *>(uuid);
    }
}
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_VALID_RANGE
static uint8_t essValueBytes(uint8_t type)
{
  switch (type)
    {
      case CProtoNimblePrphEss::PRPH_ESS_TYPE_TEMP:
      case CProtoNimblePrphEss::PRPH_ESS_TYPE_HUM:
      case CProtoNimblePrphEss::PRPH_ESS_TYPE_GAS:
        return 2;

      case CProtoNimblePrphEss::PRPH_ESS_TYPE_PRESS:
        return 4;

      case CProtoNimblePrphEss::PRPH_ESS_TYPE_UVIDX:
        return 1;

      case CProtoNimblePrphEss::PRPH_ESS_TYPE_LIGHT:
        return 3;

      default:
        return 0;
    }
}
#  endif

static int essReadFieldU32(CIOCommon *io, io_ddata_t *data, uint32_t *value)
{
  int ret;

  if (io == nullptr)
    {
      *value = 0;
      return OK;
    }

  ret = io->getData(*data, 1);
  if (ret < 0)
    {
      return ret;
    }

  switch (io->getDtype())
    {
      case SObjectId::DTYPE_UINT8:
        *value = data->get<uint8_t>(0);
        break;

      case SObjectId::DTYPE_UINT16:
        *value = data->get<uint16_t>(0);
        break;

      case SObjectId::DTYPE_UINT32:
        *value = data->get<uint32_t>(0);
        break;

      default:
        return -ENOTSUP;
    }

  return OK;
}

static int essReadFieldRaw(CIOCommon *io, io_ddata_t *data, uint8_t *value, uint8_t *len)
{
  int ret;

  if (io == nullptr)
    {
      *len = 0;
      return OK;
    }

  if (io->getDataSize() > CProtoNimblePrphEss::ESS_RAW_DESCRIPTOR_MAX)
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
int CProtoNimblePrphEss::notifierCb(void *priv, io_ddata_t *data)
{
  SPrphNotiferCb *ncb = (SPrphNotiferCb *)priv;

  if (ncb == nullptr)
    {
      DAWNERR("NULL ncb pointer in notifier\n");
      return -EINVAL;
    }

  CIOCommon *io = (CIOCommon *)ncb->io;

  if (io == nullptr)
    {
      DAWNERR("NULL io pointer in notifier\n");
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
int CProtoNimblePrphEss::descriptorCb(uint16_t conn_handle,
                                      uint16_t attr_handle,
                                      struct ble_gatt_access_ctxt *ctxt,
                                      void *arg)
{
  SEssDscCb *dcb = static_cast<SEssDscCb *>(arg);
  int ret;

  (void)conn_handle;
  (void)attr_handle;

  if (dcb == nullptr || ctxt->op != BLE_GATT_ACCESS_OP_READ_DSC)
    {
      return BLE_ATT_ERR_UNLIKELY;
    }

  if (dcb->kind == ESS_DESC_MEASUREMENT)
    {
      uint32_t values[6];

      for (size_t i = 0; i < 6; i++)
        {
          ret = essReadFieldU32(dcb->measurement[i].io, dcb->measurement[i].data, &values[i]);
          if (ret < 0)
            {
              DAWNERR("ESS measurement descriptor field read failed %d\n", ret);
              return BLE_ATT_ERR_UNLIKELY;
            }
        }

      dcb->data[0] = static_cast<uint8_t>(values[0] & 0xff);
      dcb->data[1] = static_cast<uint8_t>((values[0] >> 8) & 0xff);
      dcb->data[2] = static_cast<uint8_t>(values[1] & 0xff);
      dcb->data[3] = static_cast<uint8_t>(values[2] & 0xff);
      dcb->data[4] = static_cast<uint8_t>((values[2] >> 8) & 0xff);
      dcb->data[5] = static_cast<uint8_t>((values[2] >> 16) & 0xff);
      dcb->data[6] = static_cast<uint8_t>(values[3] & 0xff);
      dcb->data[7] = static_cast<uint8_t>((values[3] >> 8) & 0xff);
      dcb->data[8] = static_cast<uint8_t>((values[3] >> 16) & 0xff);
      dcb->data[9] = static_cast<uint8_t>(values[4] & 0xff);
      dcb->data[10] = static_cast<uint8_t>(values[5] & 0xff);
    }
  else if (dcb->kind == ESS_DESC_CONFIGURATION || dcb->kind == ESS_DESC_TRIGGER_SETTING)
    {
      ret = essReadFieldRaw(dcb->field.io, dcb->field.data, dcb->data, &dcb->len);
      if (ret < 0)
        {
          DAWNERR("ESS raw descriptor field read failed %d\n", ret);
          return BLE_ATT_ERR_UNLIKELY;
        }
    }

  ret = os_mbuf_append(ctxt->om, dcb->data, dcb->len);
  if (ret < 0)
    {
      DAWNERR("ESS descriptor append failed %d\n", ret);
      return BLE_ATT_ERR_UNLIKELY;
    }

  return 0;
}
#endif

// NOTE: For digial IO - 1B data are supported For analog IO - 4B data are
// supported
template<typename T, size_t WriteBytes>
int CProtoNimblePrphEss::callback(uint16_t conn_handle,
                                  uint16_t attr_handle,
                                  struct ble_gatt_access_ctxt *ctxt,
                                  void *arg)
{
  SPrphNotiferCb *ncb = static_cast<SPrphNotiferCb *>(arg);
  CIOCommon *io = static_cast<CIOCommon *>(ncb->io);
  uint16_t uuid16 = ble_uuid_u16(ctxt->chr->uuid);
  int ret;

  switch (ctxt->op)
    {
      case BLE_GATT_ACCESS_OP_READ_CHR:
        {
          uint32_t retval;

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

          // Convert units if this is float type

          if (io->getDtype() == SObjectId::DTYPE_FLOAT)
            {
              float tmp;

              // Convert unit

              tmp = ncb->data->get<float>(0) * IProtoNimblePrphCb::charScaleGet(uuid16);

              // Convert data to int16_t

              retval = (T)tmp;
            }
          else
            {
              DAWNERR("ESS data type not supported yet\n");
              return BLE_ATT_ERR_UNLIKELY;
            }

          // Write data (WriteBytes may be smaller than sizeof(T) for
          // packed types like uint24)

          ret = os_mbuf_append(ctxt->om, &retval, WriteBytes);
          if (ret < 0)
            {
              DAWNERR("os_mbuf_append failed %d\n", ret);
            }

          return 0;
        }

      default:
        {
          DAWNERR("ESS GATT operation not supported\n");
          return BLE_ATT_ERR_UNLIKELY;
        }
    }
}

int CProtoNimblePrphEss::allocESS()
{
  // Configure service

  svc.type = BLE_GATT_SVC_TYPE_PRIMARY;
  svc.uuid = reinterpret_cast<const ble_uuid_t *>(IProtoNimblePrphCb::UUID_ESS);
  svc.includes = nullptr;

  // One more characteristic for end of array

  noChar = cb->getObjectsLen() + 1;

  // Allocate memory for characterisitcs

  svc.characteristics = new (std::nothrow) ble_gatt_chr_def[noChar]();

  if (svc.characteristics == nullptr)
    {
      DAWNERR("Failed to allocate characteristics\n");
      return -ENOMEM;
    }

  return OK;
}

int CProtoNimblePrphEss::createESS()
{
  const uint32_t *char_uuid = nullptr;
  int j = 0;
  int ret;

  if (svc.characteristics == nullptr)
    {
      DAWNERR("Failed to allocate characteristics\n");
      return -ENOMEM;
    }

  // Configure service

  svc.type = BLE_GATT_SVC_TYPE_PRIMARY;
  svc.uuid = reinterpret_cast<const ble_uuid_t *>(IProtoNimblePrphCb::UUID_ESS);
  svc.includes = nullptr;

  for (auto const objid : vio)
    {
      CIOCommon *io = cb->getObject(objid);

      // NOTE: NimBLE C API declares svc.characteristics as
      // const ble_gatt_chr_def*, but Dawn allocates it dynamically
      // and needs write access to populate fields at runtime.

      struct ble_gatt_chr_def *chr = const_cast<struct ble_gatt_chr_def *>(&svc.characteristics[j]);

      if (io == nullptr)
        {
          DAWNERR("ESS IO not found\n");
          return -EIO;
        }

      // Seekable IOs not supported by ESS

      if (io->isSeekable())
        {
          DAWNERR("seekable IO not supported by ESS"
                  " (objid=0x%08" PRIx32 ")\n",
                  io->getIdV());
          return -ENOTSUP;
        }

      SPrphNotiferCb *ncb = nimbleGattNotifierCreate(io);
      if (ncb == nullptr)
        {
          DAWNERR("ESS notifier allocation failed\n");
          return -ENOMEM;
        }

      // Fill characteristic, access_cb is fill later

      nimbleGattChrInit(*chr, ncb);
      char_uuid = nullptr;

      // Check characteristic properties

#ifdef CONFIG_DAWN_IO_NOTIFY
      ret = nimbleGattChrNotifySet(*chr, *io, *ncb, notifierCb, objid);
      if (ret < 0)
        {
          DAWNERR("ESS notifier setup failed\n");
        }
#endif

      nimbleGattChrAccessSet(*chr, *io);

      // Get IO class

      switch (ioTypeMap[objid])
        {
          case PRPH_ESS_TYPE_TEMP:
            {
              chr->access_cb = CProtoNimblePrphEss::callback<int16_t>;
              char_uuid = IProtoNimblePrphCb::UUID_TEMP;
              break;
            }

          case PRPH_ESS_TYPE_HUM:
            {
              chr->access_cb = CProtoNimblePrphEss::callback<int16_t>;
              char_uuid = IProtoNimblePrphCb::UUID_HUM;
              break;
            }

          case PRPH_ESS_TYPE_PRESS:
            {
              chr->access_cb = CProtoNimblePrphEss::callback<uint32_t>;
              char_uuid = IProtoNimblePrphCb::UUID_PRESS;
              break;
            }

          case PRPH_ESS_TYPE_UVIDX:
            {
              chr->access_cb = CProtoNimblePrphEss::callback<uint8_t>;
              char_uuid = IProtoNimblePrphCb::UUID_UVIDX;
              break;
            }

          case PRPH_ESS_TYPE_GAS:
            {
              // Non-standard UUID: 0x272A is not assigned by the BT SIG.

              chr->access_cb = CProtoNimblePrphEss::callback<int16_t>;
              char_uuid = IProtoNimblePrphCb::UUID_RESISTANCE;
              break;
            }

          case PRPH_ESS_TYPE_LIGHT:
            {
              // Illuminance is a 24-bit unsigned (BT SIG 0x2AFB)

              chr->access_cb = CProtoNimblePrphEss::callback<uint32_t, 3>;
              char_uuid = IProtoNimblePrphCb::UUID_ILLUMINANCE;
              break;
            }

          default:
            {
              DAWNERR("unknown ESS type %d\n", ioTypeMap[objid]);
              nimbleGattChrNotifierFree(*chr);
              return -EINVAL;
            }
        }

      // Assignsome dummy value

      if (char_uuid == nullptr)
        {
          DAWNERR("No UUID for ESS IO\n");
          nimbleGattChrNotifierFree(*chr);
          return -EINVAL;
        }

      ret = nimbleGattChrUuid128Set(*chr, char_uuid);
      if (ret != OK)
        {
          DAWNERR("ESS UUID allocation failed\n");
          nimbleGattChrNotifierFree(*chr);
          return ret;
        }

      ret = configureDescriptors(chr, objid, ioTypeMap[objid]);
      if (ret != OK)
        {
          nimbleGattChrUuidFree(*chr);
          nimbleGattChrNotifierFree(*chr);
          return ret;
        }

      // Next service

      j++;
    }

  return OK;
}

void CProtoNimblePrphEss::deleteESS()
{
  // NOTE: svc.characteristics fields are const-qualified per NimBLE API.
  // Dawn allocated this memory and is the rightful owner, so the cast is safe.
  if (svc.characteristics)
    {
      for (size_t i = 0; i < noChar - 1; i++)
        {
          if (svc.characteristics[i].uuid)
            {
              nimbleGattChrUuidFree(const_cast<struct ble_gatt_chr_def &>(svc.characteristics[i]));
            }

#ifdef DAWN_PROTO_NIMBLE_ESS_ANY_DESCRIPTOR
          if (svc.characteristics[i].descriptors)
            {
              struct ble_gatt_dsc_def *dsc =
                const_cast<struct ble_gatt_dsc_def *>(svc.characteristics[i].descriptors);

              for (size_t j = 0; dsc[j].uuid != nullptr; j++)
                {
                  SEssDscCb *dcb = static_cast<SEssDscCb *>(dsc[j].arg);

                  essFreeUuid(dsc[j].uuid);
                  if (dcb != nullptr)
                    {
                      if (dcb->field.data != nullptr)
                        {
                          delete dcb->field.data;
                        }
                      for (size_t k = 0; k < 6; k++)
                        {
                          if (dcb->measurement[k].data != nullptr)
                            {
                              delete dcb->measurement[k].data;
                            }
                        }
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
  created = false;
}

int CProtoNimblePrphEss::configureDescriptors(struct ble_gatt_chr_def *chr,
                                              SObjectId::ObjectId objid,
                                              uint8_t type)
{
#ifndef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
  UNUSED(chr);
  UNUSED(objid);
  UNUSED(type);
  return OK;
#else
  const auto metaIt = ioMetaMap.find(objid);
  SEssMeta *meta;
  struct ble_gatt_dsc_def *dsc;
  size_t count = 0;
  size_t idx = 0;
  UNUSED(idx);

  if (metaIt == ioMetaMap.end())
    {
      return OK;
    }

  meta = const_cast<SEssMeta *>(&metaIt->second);

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_USER_DESCRIPTION
  if (meta->desc & ESS_DESC_USER_DESCRIPTION)
    {
      count++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_VALID_RANGE
  if (meta->desc & ESS_DESC_VALID_RANGE)
    {
      if (essValueBytes(type) == 0)
        {
          DAWNERR("ESS valid range unsupported for type %u\n", type);
          return -EINVAL;
        }
      count++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_MEASUREMENT
  if (meta->desc & ESS_DESC_MEASUREMENT)
    {
      count++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_CONFIGURATION
  if (meta->desc & ESS_DESC_CONFIGURATION)
    {
      count++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_TRIGGER_SETTING
  if (meta->desc & ESS_DESC_TRIGGER_SETTING)
    {
      count++;
    }
#  endif

  if (count == 0)
    {
      return OK;
    }

  dsc = new (std::nothrow) ble_gatt_dsc_def[count + 1]();
  if (dsc == nullptr)
    {
      return -ENOMEM;
    }

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_USER_DESCRIPTION
  if (meta->desc & ESS_DESC_USER_DESCRIPTION)
    {
      SEssDscCb *dcb = new (std::nothrow) SEssDscCb{};
      ble_uuid_any_t *uuid = essMakeUuid16(UUID16_CHR_USER_DESCRIPTION);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          essFreeUuid(uuid ? &uuid->u : nullptr);
          delete[] dsc;
          return -ENOMEM;
        }

      dcb->kind = ESS_DESC_USER_DESCRIPTION;
      dcb->len = 0;
      while (dcb->len < ESS_USER_DESCRIPTION_MAX && meta->userDescription[dcb->len] != '\0')
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

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_VALID_RANGE
  if (meta->desc & ESS_DESC_VALID_RANGE)
    {
      SEssDscCb *dcb = new (std::nothrow) SEssDscCb{};
      ble_uuid_any_t *uuid = essMakeUuid16(UUID16_VALID_RANGE);
      uint8_t width = essValueBytes(type);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          essFreeUuid(uuid ? &uuid->u : nullptr);
          delete[] dsc;
          return -ENOMEM;
        }

      dcb->kind = ESS_DESC_VALID_RANGE;
      dcb->len = width * 2;
      for (uint8_t i = 0; i < width; i++)
        {
          dcb->data[i] = static_cast<uint8_t>((meta->validMin >> (8 * i)) & 0xff);
          dcb->data[width + i] = static_cast<uint8_t>((meta->validMax >> (8 * i)) & 0xff);
        }

      dsc[idx].uuid = &uuid->u;
      dsc[idx].att_flags = BLE_ATT_F_READ;
      dsc[idx].access_cb = descriptorCb;
      dsc[idx].arg = dcb;
      idx++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_CONFIGURATION
  if (meta->desc & ESS_DESC_CONFIGURATION)
    {
      SEssDscCb *dcb = new (std::nothrow) SEssDscCb{};
      ble_uuid_any_t *uuid = essMakeUuid16(UUID16_ES_CONFIGURATION);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          essFreeUuid(uuid ? &uuid->u : nullptr);
          delete[] dsc;
          return -ENOMEM;
        }

      dcb->kind = ESS_DESC_CONFIGURATION;
      dcb->field.io = cb->getObject(meta->configurationObjid);
      if (dcb->field.io == nullptr)
        {
          delete dcb;
          essFreeUuid(&uuid->u);
          delete[] dsc;
          return -EIO;
        }

      dcb->field.data = dcb->field.io->ddata_alloc(1);
      if (dcb->field.data == nullptr)
        {
          delete dcb;
          essFreeUuid(&uuid->u);
          delete[] dsc;
          return -ENOMEM;
        }

      dsc[idx].uuid = &uuid->u;
      dsc[idx].att_flags = BLE_ATT_F_READ;
      dsc[idx].access_cb = descriptorCb;
      dsc[idx].arg = dcb;
      idx++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_MEASUREMENT
  if (meta->desc & ESS_DESC_MEASUREMENT)
    {
      SEssDscCb *dcb = new (std::nothrow) SEssDscCb{};
      ble_uuid_any_t *uuid = essMakeUuid16(UUID16_ES_MEASUREMENT);
      SObjectId::ObjectId objids[6] = {
        meta->measurementFlagsObjid,
        meta->samplingFunctionObjid,
        meta->measurementPeriodObjid,
        meta->updateIntervalObjid,
        meta->applicationObjid,
        meta->uncertaintyObjid,
      };
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          essFreeUuid(uuid ? &uuid->u : nullptr);
          delete[] dsc;
          return -ENOMEM;
        }

      dcb->kind = ESS_DESC_MEASUREMENT;
      dcb->len = 11;
      for (size_t i = 0; i < 6; i++)
        {
          if (objids[i] == 0)
            {
              continue;
            }

          dcb->measurement[i].io = cb->getObject(objids[i]);
          if (dcb->measurement[i].io == nullptr)
            {
              for (size_t k = 0; k < 6; k++)
                {
                  delete dcb->measurement[k].data;
                }
              delete dcb;
              essFreeUuid(&uuid->u);
              delete[] dsc;
              return -EIO;
            }

          dcb->measurement[i].data = dcb->measurement[i].io->ddata_alloc(1);
          if (dcb->measurement[i].data == nullptr)
            {
              for (size_t k = 0; k < 6; k++)
                {
                  delete dcb->measurement[k].data;
                }
              delete dcb;
              essFreeUuid(&uuid->u);
              delete[] dsc;
              return -ENOMEM;
            }
        }

      dsc[idx].uuid = &uuid->u;
      dsc[idx].att_flags = BLE_ATT_F_READ;
      dsc[idx].access_cb = descriptorCb;
      dsc[idx].arg = dcb;
      idx++;
    }
#  endif

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_TRIGGER_SETTING
  if (meta->desc & ESS_DESC_TRIGGER_SETTING)
    {
      SEssDscCb *dcb = new (std::nothrow) SEssDscCb{};
      ble_uuid_any_t *uuid = essMakeUuid16(UUID16_ES_TRIGGER_SETTING);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          essFreeUuid(uuid ? &uuid->u : nullptr);
          delete[] dsc;
          return -ENOMEM;
        }

      dcb->kind = ESS_DESC_TRIGGER_SETTING;
      dcb->field.io = cb->getObject(meta->triggerSettingObjid);
      if (dcb->field.io == nullptr)
        {
          delete dcb;
          essFreeUuid(&uuid->u);
          delete[] dsc;
          return -EIO;
        }

      dcb->field.data = dcb->field.io->ddata_alloc(1);
      if (dcb->field.data == nullptr)
        {
          delete dcb;
          essFreeUuid(&uuid->u);
          delete[] dsc;
          return -ENOMEM;
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
#endif
}

void CProtoNimblePrphEss::allocObject(const SProtoNimblePrphIOBindEssObjid &obj,
                                      const uint32_t *ext)
{
  uint8_t type = cfgIdIOBindEssCfgObjTypeGet(obj.cfg);
#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
  SEssMeta meta{};
#endif

  DAWNINFO("ESS allocate type=%d object 0x%" PRIx32 "\n", type, obj.objid.v);

  // Store type for later

  ioTypeMap.insert_or_assign(obj.objid.v, type);

#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
  for (size_t i = 0; i < obj.extCount; i++)
    {
      uint8_t kind = cfgIdIOBindEssExtKindGet(ext[0]);
      uint8_t size = cfgIdIOBindEssExtSizeGet(ext[0]);
      const uint32_t *data = &ext[1];
      UNUSED(data);

      switch (kind)
        {
          case ESS_EXT_USER_DESCRIPTION:
            {
              size_t bytes = static_cast<size_t>(size) * sizeof(uint32_t);
              bytes = bytes < ESS_USER_DESCRIPTION_MAX ? bytes : ESS_USER_DESCRIPTION_MAX;

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_USER_DESCRIPTION
              meta.desc |= ESS_DESC_USER_DESCRIPTION;
              std::memcpy(meta.userDescription, data, bytes);
#  endif
              break;
            }

          case ESS_EXT_VALID_RANGE:
            {
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_VALID_RANGE
              if (size >= 2)
                {
                  meta.desc |= ESS_DESC_VALID_RANGE;
                  meta.validMin = data[0];
                  meta.validMax = data[1];
                }
#  endif
              break;
            }

          case ESS_EXT_MEASUREMENT:
            {
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_MEASUREMENT
              if (size >= 6)
                {
                  meta.desc |= ESS_DESC_MEASUREMENT;
                  meta.measurementFlagsObjid = data[0];
                  meta.samplingFunctionObjid = data[1];
                  meta.measurementPeriodObjid = data[2];
                  meta.updateIntervalObjid = data[3];
                  meta.applicationObjid = data[4];
                  meta.uncertaintyObjid = data[5];
                }
#  endif
              break;
            }

          case ESS_EXT_CONFIGURATION:
            {
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_CONFIGURATION
              if (size >= 1)
                {
                  meta.desc |= ESS_DESC_CONFIGURATION;
                  meta.configurationObjid = data[0];
                }
#  endif
              break;
            }

          case ESS_EXT_TRIGGER_SETTING:
            {
#  ifdef CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_TRIGGER_SETTING
              if (size >= 1)
                {
                  meta.desc |= ESS_DESC_TRIGGER_SETTING;
                  meta.triggerSettingObjid = data[0];
                }
#  endif
              break;
            }

          default:
            {
              DAWNERR("unknown ESS extension kind %u\n", kind);
              break;
            }
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
  if (meta.measurementFlagsObjid != 0)
    {
      cb->regObject(meta.measurementFlagsObjid);
    }

  if (meta.samplingFunctionObjid != 0)
    {
      cb->regObject(meta.samplingFunctionObjid);
    }

  if (meta.measurementPeriodObjid != 0)
    {
      cb->regObject(meta.measurementPeriodObjid);
    }

  if (meta.updateIntervalObjid != 0)
    {
      cb->regObject(meta.updateIntervalObjid);
    }

  if (meta.applicationObjid != 0)
    {
      cb->regObject(meta.applicationObjid);
    }

  if (meta.uncertaintyObjid != 0)
    {
      cb->regObject(meta.uncertaintyObjid);
    }

  if (meta.configurationObjid != 0)
    {
      cb->regObject(meta.configurationObjid);
    }

  if (meta.triggerSettingObjid != 0)
    {
      cb->regObject(meta.triggerSettingObjid);
    }
#endif

  vio.push_back(obj.objid.v);
}

// Assumption: Configuration item passed by caller is a valid type supported by
// this class.
void CProtoNimblePrphEss::configureDesc(const SObjectCfg::SObjectCfgItem *item)
{
  // Iterrate over objectid list

  for (size_t k = 0; k < item->cfgid.s.size;)
    {
      const SProtoNimblePrphIOBindEss *tmp =
        reinterpret_cast<const SProtoNimblePrphIOBindEss *>(&item->data[k]);

      uint8_t size = cfgIdIOBindEssCfg0SizeGet(tmp->cfg0);

      size_t pos = k + sizeof(SProtoNimblePrphIOBindEss) / sizeof(uint32_t);

      for (size_t i = 0; i < size; i++)
        {
          const SProtoNimblePrphIOBindEssObjid *obj =
            reinterpret_cast<const SProtoNimblePrphIOBindEssObjid *>(&item->data[pos]);
          const uint32_t *ext =
            &item->data[pos + sizeof(SProtoNimblePrphIOBindEssObjid) / sizeof(uint32_t)];

          allocObject(*obj, ext);

          pos += sizeof(SProtoNimblePrphIOBindEssObjid) / sizeof(uint32_t);
          for (size_t j = 0; j < obj->extCount; j++)
            {
              pos += 1 + cfgIdIOBindEssExtSizeGet(item->data[pos]);
            }
        }

      k = pos;
    }
}

CProtoNimblePrphEss::CProtoNimblePrphEss(const SObjectCfg::SObjectCfgItem *desc_,
                                         IProtoNimblePrphCb *cb_)
  : IProtoNimblePrphService(desc_, cb_)
  , created(false)
{
  // Service ID not assigned yet

  id = -1;
  noChar = 0;
}

CProtoNimblePrphEss::~CProtoNimblePrphEss()
{
  deinit();
}

int CProtoNimblePrphEss::init()
{
  int ret;

  // Configure object

  configureDesc(desc);

  // Allocate service

  ret = allocESS();
  if (ret != OK)
    {
      return ret;
    }

  // Register servie in peripheral handler

  id = cb->serviceRegister(&svc);
  if (id < 0)
    {
      DAWNERR("ESS service registration failed\n");
      return -EIO;
    }

  return OK;
}

int CProtoNimblePrphEss::deinit()
{
  deleteESS();
  return OK;
}

int CProtoNimblePrphEss::start()
{
  int ret;

  if (created)
    {
      return cb->startService(id);
    }

  // Create service

  ret = createESS();
  if (ret != OK)
    {
      deleteESS();
      allocESS();
      return ret;
    }

  created = true;

  // Signal start to peripheral handler

  return cb->startService(id);
}

int CProtoNimblePrphEss::stop()
{
  // Signal stop to peripheral handler

  return cb->stopService(id);
}
