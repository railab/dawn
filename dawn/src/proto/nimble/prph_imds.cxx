// dawn/src/proto/nimble/prph_imds.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/prph_imds.hxx"

#include <cstdlib>
#include <new>

#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/proto/nimble/gatt_runtime.hxx"
#include "host/ble_hs_mbuf.h"
#include "os/os_mbuf.h"

using namespace dawn;

#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
static ble_uuid_any_t *imdsMakeUuid16(uint16_t value)
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

static void imdsFreeUuid(const ble_uuid_t *uuid)
{
  if (uuid != nullptr)
    {
      delete reinterpret_cast<const ble_uuid_any_t *>(uuid);
    }
}
#endif

#ifdef CONFIG_DAWN_IO_NOTIFY
int CProtoNimblePrphImds::notifierCb(void *priv, io_ddata_t *data)
{
  SPrphNotiferCb *ncb = static_cast<SPrphNotiferCb *>(priv);

  if (ncb == nullptr)
    {
      DAWNERR("NULL ncb pointer in notifier\n");
      return -EINVAL;
    }

  CIOCommon *io = reinterpret_cast<CIOCommon *>(ncb->io);

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
int CProtoNimblePrphImds::descriptorCb(uint16_t conn_handle,
                                       uint16_t attr_handle,
                                       struct ble_gatt_access_ctxt *ctxt,
                                       void *arg)
{
  SImdsDscCb *dcb = static_cast<SImdsDscCb *>(arg);
  int ret;

  UNUSED(conn_handle);
  UNUSED(attr_handle);

  if (dcb == nullptr || ctxt->op != BLE_GATT_ACCESS_OP_READ_DSC)
    {
      return BLE_ATT_ERR_UNLIKELY;
    }

  ret = os_mbuf_append(ctxt->om, dcb->data, dcb->len);
  if (ret < 0)
    {
      DAWNERR("IMDS descriptor append failed %d\n", ret);
      return BLE_ATT_ERR_UNLIKELY;
    }

  return 0;
}
#endif

// NOTE: For digial IO - 1B data are supported For analog IO - 4B data are
// supported
template<typename T, size_t WriteBytes>
int CProtoNimblePrphImds::callback(uint16_t conn_handle,
                                   uint16_t attr_handle,
                                   struct ble_gatt_access_ctxt *ctxt,
                                   void *arg)
{
  SPrphNotiferCb *ncb = static_cast<SPrphNotiferCb *>(arg);
  CIOCommon *io = reinterpret_cast<CIOCommon *>(ncb->io);
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
              DAWNERR("IMDS data type not supported yet\n");
              return BLE_ATT_ERR_UNLIKELY;
            }

          // Write data

          ret = os_mbuf_append(ctxt->om, &retval, WriteBytes);
          if (ret < 0)
            {
              DAWNERR("os_mbuf_append failed %d\n", ret);
            }

          return 0;
        }

      default:
        {
          DAWNERR("IMDS GATT operation not supported\n");
          return BLE_ATT_ERR_UNLIKELY;
        }
    }
}

int CProtoNimblePrphImds::allocIMDS()
{
  // Configure service

  svc.type = BLE_GATT_SVC_TYPE_PRIMARY;
  svc.uuid = reinterpret_cast<const ble_uuid_t *>(IProtoNimblePrphCb::UUID_IMDS);
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

int CProtoNimblePrphImds::createIMDS()
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
  svc.uuid = reinterpret_cast<const ble_uuid_t *>(IProtoNimblePrphCb::UUID_IMDS);
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
          DAWNERR("IMDS IO not found\n");
          return -EIO;
        }

      // Seekable IOs not supported by IMDS

      if (io->isSeekable())
        {
          DAWNERR("seekable IO not supported by IMDS"
                  " (objid=0x%08" PRIx32 ")\n",
                  io->getIdV());
          return -ENOTSUP;
        }

      SPrphNotiferCb *ncb = nimbleGattNotifierCreate(io);
      if (ncb == nullptr)
        {
          DAWNERR("IMDS notifier allocation failed\n");
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
          DAWNERR("IMDS notifier setup failed\n");
        }
#endif

      nimbleGattChrAccessSet(*chr, *io);

      // Get IO class

      switch (ioTypeMap[objid])
        {
          case PRPH_IMDS_TYPE_TEMP:
            {
              chr->access_cb = CProtoNimblePrphImds::callback<int16_t>;
              char_uuid = IProtoNimblePrphCb::UUID_TEMP;
              break;
            }

          case PRPH_IMDS_TYPE_HUM:
            {
              chr->access_cb = CProtoNimblePrphImds::callback<int16_t>;
              char_uuid = IProtoNimblePrphCb::UUID_HUM;
              break;
            }

          case PRPH_IMDS_TYPE_PRESS:
            {
              chr->access_cb = CProtoNimblePrphImds::callback<uint32_t>;
              char_uuid = IProtoNimblePrphCb::UUID_PRESS;
              break;
            }

          case PRPH_IMDS_TYPE_UVIDX:
            {
              chr->access_cb = CProtoNimblePrphImds::callback<uint8_t>;
              char_uuid = IProtoNimblePrphCb::UUID_UVIDX;
              break;
            }

          case PRPH_IMDS_TYPE_GAS:
            {
              // Non-standard UUID: 0x272A is not assigned by the BT SIG.

              chr->access_cb = CProtoNimblePrphImds::callback<int16_t>;
              char_uuid = IProtoNimblePrphCb::UUID_RESISTANCE;
              break;
            }

          case PRPH_IMDS_TYPE_LIGHT:
            {
              // Illuminance is a 24-bit unsigned (BT SIG 0x2AFB)

              chr->access_cb = CProtoNimblePrphImds::callback<uint32_t, 3>;
              char_uuid = IProtoNimblePrphCb::UUID_ILLUMINANCE;
              break;
            }

          default:
            {
              DAWNERR("unknown IMDS type %d\n", ioTypeMap[objid]);
              nimbleGattChrNotifierFree(*chr);
              return -EINVAL;
            }
        }

      // Assignsome dummy value

      if (char_uuid == nullptr)
        {
          DAWNERR("No UUID for IMDS IO\n");
          nimbleGattChrNotifierFree(*chr);
          return -EINVAL;
        }

      ret = nimbleGattChrUuid128Set(*chr, char_uuid);
      if (ret != OK)
        {
          DAWNERR("IMDS UUID allocation failed\n");
          nimbleGattChrNotifierFree(*chr);
          return ret;
        }

      ret = configureDescriptors(chr, objid);
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

void CProtoNimblePrphImds::deleteIMDS()
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

#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
          if (svc.characteristics[i].descriptors)
            {
              struct ble_gatt_dsc_def *dsc =
                const_cast<struct ble_gatt_dsc_def *>(svc.characteristics[i].descriptors);

              for (size_t j = 0; dsc[j].uuid != nullptr; j++)
                {
                  SImdsDscCb *dcb = static_cast<SImdsDscCb *>(dsc[j].arg);

                  imdsFreeUuid(dsc[j].uuid);
                  delete dcb;
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

int CProtoNimblePrphImds::configureDescriptors(struct ble_gatt_chr_def *chr,
                                               SObjectId::ObjectId objid)
{
#ifndef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
  UNUSED(chr);
  UNUSED(objid);
  return OK;
#else
  const auto metaIt = ioMetaMap.find(objid);
  SImdsMeta *meta;
  struct ble_gatt_dsc_def *dsc;
  size_t count = 0;
  size_t idx = 0;

  if (metaIt == ioMetaMap.end())
    {
      return OK;
    }

  meta = const_cast<SImdsMeta *>(&metaIt->second);

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_IMDS_DESC_USER_DESCRIPTION
  if (meta->desc & IMDS_DESC_USER_DESCRIPTION)
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

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_IMDS_DESC_USER_DESCRIPTION
  if (meta->desc & IMDS_DESC_USER_DESCRIPTION)
    {
      SImdsDscCb *dcb = new (std::nothrow) SImdsDscCb{};
      ble_uuid_any_t *uuid = imdsMakeUuid16(UUID16_CHR_USER_DESCRIPTION);
      if (dcb == nullptr || uuid == nullptr)
        {
          delete dcb;
          imdsFreeUuid(uuid ? &uuid->u : nullptr);
          delete[] dsc;
          return -ENOMEM;
        }

      dcb->kind = IMDS_DESC_USER_DESCRIPTION;
      dcb->len = 0;
      while (dcb->len < IMDS_USER_DESCRIPTION_MAX && meta->userDescription[dcb->len] != '\0')
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

  chr->descriptors = dsc;
  return OK;
#endif
}

void CProtoNimblePrphImds::allocObject(const SProtoNimblePrphIOBindImdsObjid &obj,
                                       const uint32_t *ext)
{
  uint8_t type = cfgIdIOBindImdsCfgObjTypeGet(obj.cfg);
#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
  SImdsMeta meta{};
#endif

  DAWNINFO("IMDS allocate type=%d object 0x%" PRIx32 "\n", type, obj.objid.v);

  // Store type for later

  ioTypeMap.insert_or_assign(obj.objid.v, type);

#ifdef CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA
  for (size_t i = 0; i < obj.extCount; i++)
    {
      uint8_t kind = cfgIdIOBindImdsExtKindGet(ext[0]);
      uint8_t size = cfgIdIOBindImdsExtSizeGet(ext[0]);
      const uint32_t *data = &ext[1];

      switch (kind)
        {
          case IMDS_EXT_USER_DESCRIPTION:
            {
              size_t bytes = static_cast<size_t>(size) * sizeof(uint32_t);
              bytes = bytes < IMDS_USER_DESCRIPTION_MAX ? bytes : IMDS_USER_DESCRIPTION_MAX;

#  ifdef CONFIG_DAWN_PROTO_NIMBLE_IMDS_DESC_USER_DESCRIPTION
              meta.desc |= IMDS_DESC_USER_DESCRIPTION;
              std::memcpy(meta.userDescription, data, bytes);
#  endif
              break;
            }

          default:
            {
              DAWNERR("unknown IMDS extension kind %u\n", kind);
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

  vio.push_back(obj.objid.v);
}

// Assumption: Configuration item passed by caller is a valid type supported by
// this class.
void CProtoNimblePrphImds::configureDesc(const SObjectCfg::SObjectCfgItem *item)
{
  // Iterrate over objectid list

  for (size_t k = 0; k < item->cfgid.s.size;)
    {
      const SProtoNimblePrphIOBindImds *tmp =
        reinterpret_cast<const SProtoNimblePrphIOBindImds *>(&item->data[k]);

      uint8_t size = cfgIdIOBindImdsCfg0SizeGet(tmp->cfg0);

      size_t pos = k + sizeof(SProtoNimblePrphIOBindImds) / sizeof(uint32_t);

      for (size_t i = 0; i < size; i++)
        {
          const SProtoNimblePrphIOBindImdsObjid *obj =
            reinterpret_cast<const SProtoNimblePrphIOBindImdsObjid *>(&item->data[pos]);
          const uint32_t *ext =
            &item->data[pos + sizeof(SProtoNimblePrphIOBindImdsObjid) / sizeof(uint32_t)];

          // Allocate object

          allocObject(*obj, ext);

          pos += sizeof(SProtoNimblePrphIOBindImdsObjid) / sizeof(uint32_t);
          for (size_t j = 0; j < obj->extCount; j++)
            {
              pos += 1 + cfgIdIOBindImdsExtSizeGet(item->data[pos]);
            }
        }

      k = pos;
    }
}

CProtoNimblePrphImds::CProtoNimblePrphImds(const SObjectCfg::SObjectCfgItem *desc_,
                                           IProtoNimblePrphCb *cb_)
  : IProtoNimblePrphService(desc_, cb_)
  , created(false)
{
  // Service ID not assigned yet

  id = -1;
  noChar = 0;
}

CProtoNimblePrphImds::~CProtoNimblePrphImds()
{
  deinit();
}

int CProtoNimblePrphImds::init()
{
  int ret;

  // Configure object

  configureDesc(desc);

  // Allocate service

  ret = allocIMDS();
  if (ret != OK)
    {
      return ret;
    }

  // Register servie in peripheral handler

  id = cb->serviceRegister(&svc);
  if (id < 0)
    {
      DAWNERR("IMDS service registration failed\n");
      return -EIO;
    }

  return OK;
}

int CProtoNimblePrphImds::deinit()
{
  deleteIMDS();
  return OK;
}

int CProtoNimblePrphImds::start()
{
  int ret;

  if (created)
    {
      return cb->startService(id);
    }

  // Create service

  ret = createIMDS();
  if (ret != OK)
    {
      deleteIMDS();
      allocIMDS();
      return ret;
    }

  created = true;

  // Signal start to peripheral handler

  return cb->startService(id);
}

int CProtoNimblePrphImds::stop()
{
  // Signal stop to peripheral handler

  return cb->stopService(id);
}
