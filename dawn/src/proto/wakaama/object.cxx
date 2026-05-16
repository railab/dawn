// dawn/src/proto/wakaama/object.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "object_binding.hxx"

#include <cerrno>
#include <cinttypes>
#include <cstring>
#include <limits>
#include <new>

#if defined(CONFIG_DAWN_DTYPE_B16) || defined(CONFIG_DAWN_DTYPE_UB16)
#  include "dawn/porting/fixedmath.hxx"
#endif

using namespace dawn;
using namespace dawn::wakaama_internal;

namespace
{
template<typename T>
T readScalar(const void *ptr)
{
  T val;

  std::memcpy(&val, ptr, sizeof(T));
  return val;
}

template<typename T>
int storeSigned(void *ptr, int64_t val)
{
  if (val < static_cast<int64_t>(std::numeric_limits<T>::min()) ||
      val > static_cast<int64_t>(std::numeric_limits<T>::max()))
    {
      return -ERANGE;
    }

  *static_cast<T *>(ptr) = static_cast<T>(val);
  return OK;
}

template<typename T>
int storeUnsigned(void *ptr, uint64_t val)
{
  if (val > static_cast<uint64_t>(std::numeric_limits<T>::max()))
    {
      return -ERANGE;
    }

  *static_cast<T *>(ptr) = static_cast<T>(val);
  return OK;
}

template<typename T>
int decodeAndStoreSigned(void *ptr, const lwm2m_data_t &data)
{
  int64_t val = 0;

  if (!lwm2m_data_decode_int(&data, &val))
    {
      return -EINVAL;
    }

  return storeSigned<T>(ptr, val);
}

template<typename T>
int decodeAndStoreUnsigned(void *ptr, const lwm2m_data_t &data)
{
  uint64_t val = 0;

  if (!lwm2m_data_decode_uint(&data, &val))
    {
      return -EINVAL;
    }

  return storeUnsigned<T>(ptr, val);
}

#ifdef CONFIG_DAWN_DTYPE_BLOCK
bool bufferValue(const lwm2m_data_t &data, const uint8_t **buffer, size_t *length)
{
  if (buffer == nullptr || length == nullptr)
    {
      return false;
    }

  switch (data.type)
    {
      case LWM2M_TYPE_STRING:
      case LWM2M_TYPE_OPAQUE:
      case LWM2M_TYPE_CORE_LINK:
        break;

      default:
        return false;
    }

  *buffer = data.value.asBuffer.buffer;
  *length = data.value.asBuffer.length;
  return *length == 0 || *buffer != nullptr;
}
#endif
} // namespace

ObjectBinding::ObjectBinding(const SObjectCfg::SObjectCfgItem *item, CProtoWakaama *proto_)
  : proto(proto_)
  , configStatus(OK)
  , lwm2m()
{
  configStatus = configureDesc(item);
}

ObjectBinding::~ObjectBinding()
{
  deinit();
}

int ObjectBinding::configureDesc(const SObjectCfg::SObjectCfgItem *item)
{
  const SProtoWakaamaIOBind *bind;
  constexpr size_t bindWords = sizeof(SProtoWakaamaIOBind) / sizeof(uint32_t);
  size_t count;

  if (item == nullptr)
    {
      return -EINVAL;
    }

  if (sizeof(SProtoWakaamaIOBind) % sizeof(uint32_t) != 0 || item->cfgid.s.size == 0 ||
      (item->cfgid.s.size % bindWords) != 0)
    {
      return -EINVAL;
    }

  std::memset(&lwm2m, 0, sizeof(lwm2m));

  count = item->cfgid.s.size / bindWords;
  bind = reinterpret_cast<const SProtoWakaamaIOBind *>(item->data);

  for (size_t i = 0; i < count; i++)
    {
      int ret = allocObject(bind[i]);
      if (ret < 0)
        {
          return ret;
        }
    }

  return OK;
}

int ObjectBinding::allocObject(const SProtoWakaamaIOBind &bind)
{
  Resource res;

  if ((bind.access & CProtoWakaama::PROTO_WAKAAMA_ACCESS_RWE) == 0 ||
      (bind.access & ~CProtoWakaama::PROTO_WAKAAMA_ACCESS_RWE) != 0)
    {
      return -EINVAL;
    }

  if (resources.empty())
    {
      lwm2m.objID = bind.objectId;
    }

  if (bind.objectId != lwm2m.objID)
    {
      DAWNERR("Wakaama object bind mixes object IDs %u and %u\n", lwm2m.objID, bind.objectId);
      return -EINVAL;
    }

  for (const Resource &existing : resources)
    {
      if (existing.instanceId == bind.instanceId && existing.resourceId == bind.resourceId)
        {
          DAWNERR("Duplicate Wakaama resource bind %u/%u/%u\n",
                  bind.objectId,
                  bind.instanceId,
                  bind.resourceId);
          return -EEXIST;
        }
    }

  res.instanceId = bind.instanceId;
  res.resourceId = bind.resourceId;
  res.access = bind.access;
  res.objid = bind.objid;
  res.io = nullptr;
  res.data = nullptr;
  resources.push_back(res);

  if (proto != nullptr)
    {
      proto->regObject(bind.objid);
    }

  return OK;
}

int ObjectBinding::init()
{
  if (configStatus < 0)
    {
      return configStatus;
    }

  if (resources.empty())
    {
      return -EINVAL;
    }

  for (Resource &res : resources)
    {
      if (proto == nullptr)
        {
          return -ENODEV;
        }

      res.io = proto->getObject(res.objid);
      if (res.io == nullptr)
        {
          return -ENOENT;
        }

      res.data = res.io->ddata_alloc(1, res.io->isSeekable() ? CProtoWakaama::IO_CHUNK_CAP : 0);
      if (res.data == nullptr)
        {
          return -ENOMEM;
        }
    }

  for (Resource &res : resources)
    {
      Instance *inst = findOrCreateInstance(res.instanceId);
      if (inst == nullptr)
        {
          return -ENOMEM;
        }

      inst->resourceCount++;
    }

  lwm2m.userData = this;
  lwm2m.readFunc = ObjectBinding::readCb;
  lwm2m.writeFunc = ObjectBinding::writeCb;
  lwm2m.discoverFunc = ObjectBinding::discoverCb;
  lwm2m.executeFunc = ObjectBinding::executeCb;

#ifdef CONFIG_DAWN_IO_NOTIFY
  setupNotifications();
#endif

  return OK;
}

int ObjectBinding::deinit()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  destroyNotifications();
#endif

  for (Resource &res : resources)
    {
      delete res.data;
      res.data = nullptr;
      res.io = nullptr;
    }

  for (Instance *inst : instances)
    {
      delete inst;
    }

  instances.clear();
  lwm2m.instanceList = nullptr;

  return OK;
}

ObjectBinding::Resource *ObjectBinding::findResource(uint16_t instanceId, uint16_t resourceId)
{
  for (Resource &res : resources)
    {
      if (res.instanceId == instanceId && res.resourceId == resourceId)
        {
          return &res;
        }
    }

  return nullptr;
}

ObjectBinding::Instance *ObjectBinding::findOrCreateInstance(uint16_t instanceId)
{
  for (Instance *inst : instances)
    {
      if (inst->id == instanceId)
        {
          return inst;
        }
    }

  Instance *inst = new (std::nothrow) Instance();
  if (inst == nullptr)
    {
      return nullptr;
    }

  inst->next = nullptr;
  inst->id = instanceId;
  inst->resourceCount = 0;
  instances.push_back(inst);
  lwm2m.instanceList = LWM2M_LIST_ADD(lwm2m.instanceList, inst);

  return inst;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
void ObjectBinding::setupNotifications()
{
  for (Resource &res : resources)
    {
      NotifyContext *ctx;
      int ret;

      if (proto == nullptr || res.io == nullptr)
        {
          continue;
        }

      if ((res.access & CProtoWakaama::PROTO_WAKAAMA_ACCESS_READ) == 0 || !res.io->isNotify())
        {
          continue;
        }

      ctx = new (std::nothrow) NotifyContext();
      if (ctx == nullptr)
        {
          DAWNERR("Failed to allocate Wakaama notify context\n");
          continue;
        }

      ctx->proto = proto;
      ctx->io = res.io;
      ctx->objectId = lwm2m.objID;
      ctx->instanceId = res.instanceId;
      ctx->resourceId = res.resourceId;
      notifyContexts.push_back(ctx);

      ret = res.io->setNotifier(notifierCb, 0, ctx);
      if (ret < 0)
        {
          DAWNERR("Wakaama setNotifier failed for objid=0x%" PRIx32 "\n", res.objid);
          delete ctx;
          notifyContexts.pop_back();
        }
    }
}

void ObjectBinding::destroyNotifications()
{
  for (NotifyContext *ctx : notifyContexts)
    {
      if (ctx != nullptr && ctx->io != nullptr && ctx->io->isNotify())
        {
          ctx->io->setNotifier(nullptr, 0, nullptr);
        }

      delete ctx;
    }

  notifyContexts.clear();
}

int ObjectBinding::notifierCb(void *priv, io_ddata_t *data)
{
  NotifyContext *ctx = static_cast<NotifyContext *>(priv);

  UNUSED(data);

  if (ctx == nullptr || ctx->proto == nullptr)
    {
      return -EINVAL;
    }

  ctx->proto->queueResourceChanged(ctx->objectId, ctx->instanceId, ctx->resourceId);

  return OK;
}
#endif

int ObjectBinding::readResource(Resource &res, lwm2m_data_t &data)
{
  CIOCommon *io;
  void *ptr;
  int ret;

  if (proto == nullptr)
    {
      return -ENODEV;
    }

  io = res.io;
  if (io == nullptr || res.data == nullptr || !io->isRead())
    {
      return -ENOENT;
    }

#ifdef CONFIG_DAWN_DTYPE_BLOCK
  if (io->getDtype() == SObjectId::DTYPE_BLOCK)
    {
      const size_t size = io->getDataSize();
      const size_t capacity = res.data->getBufferSize();

      if (size == 0)
        {
          lwm2m_data_encode_opaque(nullptr, 0, &data);
          return OK;
        }

      if (size > capacity)
        {
          return -EFBIG;
        }

      res.data->N = capacity;
      ret = io->getData(*res.data, 1);
      if (ret < 0)
        {
          return ret;
        }

      lwm2m_data_encode_opaque(static_cast<uint8_t *>(res.data->getDataPtr()), size, &data);
      return data.type == LWM2M_TYPE_OPAQUE ? OK : -ENOMEM;
    }
#endif

  ret = io->getData(*res.data, 1);
  if (ret < 0)
    {
      return ret;
    }

  ptr = res.data->getDataPtr();
  UNUSED(ptr);

  switch (io->getDtype())
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        lwm2m_data_encode_bool(readScalar<uint8_t>(ptr) != 0, &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        lwm2m_data_encode_int(readScalar<int8_t>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        lwm2m_data_encode_uint(readScalar<uint8_t>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        lwm2m_data_encode_int(readScalar<int16_t>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        lwm2m_data_encode_uint(readScalar<uint16_t>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        lwm2m_data_encode_int(readScalar<int32_t>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        lwm2m_data_encode_uint(readScalar<uint32_t>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_INT64
      case SObjectId::DTYPE_INT64:
        lwm2m_data_encode_int(readScalar<int64_t>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT64
      case SObjectId::DTYPE_UINT64:
        lwm2m_data_encode_uint(readScalar<uint64_t>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        lwm2m_data_encode_float(readScalar<float>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case SObjectId::DTYPE_DOUBLE:
        lwm2m_data_encode_float(readScalar<double>(ptr), &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
        lwm2m_data_encode_float(static_cast<double>(readScalar<int32_t>(ptr)) / 65536.0, &data);
        break;
#endif

#ifdef CONFIG_DAWN_DTYPE_UB16
      case SObjectId::DTYPE_UB16:
        lwm2m_data_encode_float(static_cast<double>(readScalar<uint32_t>(ptr)) / 65536.0, &data);
        break;
#endif

      default:
        return -ENOTSUP;
    }

  return OK;
}

int ObjectBinding::writeResource(Resource &res, const lwm2m_data_t &data)
{
  CIOCommon *io;
  void *ptr;

  if (proto == nullptr)
    {
      return -ENODEV;
    }

  io = res.io;
  if (io == nullptr || res.data == nullptr || !io->isWrite())
    {
      return -ENOENT;
    }

  ptr = res.data->getDataPtr();
  UNUSED(ptr);

  switch (io->getDtype())
    {
#ifdef CONFIG_DAWN_DTYPE_BOOL
      case SObjectId::DTYPE_BOOL:
        {
          bool bval = false;

          if (!lwm2m_data_decode_bool(&data, &bval))
            {
              return -EINVAL;
            }

          *static_cast<uint8_t *>(ptr) = bval ? 1 : 0;
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT8
      case SObjectId::DTYPE_INT8:
        {
          int ret = decodeAndStoreSigned<int8_t>(ptr, data);
          if (ret < 0)
            {
              return ret;
            }

          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT8
      case SObjectId::DTYPE_UINT8:
        {
          int ret = decodeAndStoreUnsigned<uint8_t>(ptr, data);
          if (ret < 0)
            {
              return ret;
            }

          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT16
      case SObjectId::DTYPE_INT16:
        {
          int ret = decodeAndStoreSigned<int16_t>(ptr, data);
          if (ret < 0)
            {
              return ret;
            }

          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT16
      case SObjectId::DTYPE_UINT16:
        {
          int ret = decodeAndStoreUnsigned<uint16_t>(ptr, data);
          if (ret < 0)
            {
              return ret;
            }

          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        {
          int ret = decodeAndStoreSigned<int32_t>(ptr, data);
          if (ret < 0)
            {
              return ret;
            }

          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        {
          int ret = decodeAndStoreUnsigned<uint32_t>(ptr, data);
          if (ret < 0)
            {
              return ret;
            }

          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_INT64
      case SObjectId::DTYPE_INT64:
        {
          int64_t sval = 0;

          if (!lwm2m_data_decode_int(&data, &sval))
            {
              return -EINVAL;
            }

          *static_cast<int64_t *>(ptr) = sval;
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT64
      case SObjectId::DTYPE_UINT64:
        {
          uint64_t uval = 0;

          if (!lwm2m_data_decode_uint(&data, &uval))
            {
              return -EINVAL;
            }

          *static_cast<uint64_t *>(ptr) = uval;
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        {
          double fval = 0.0;

          if (!lwm2m_data_decode_float(&data, &fval))
            {
              return -EINVAL;
            }

          *static_cast<float *>(ptr) = static_cast<float>(fval);
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_DOUBLE
      case SObjectId::DTYPE_DOUBLE:
        {
          double fval = 0.0;

          if (!lwm2m_data_decode_float(&data, &fval))
            {
              return -EINVAL;
            }

          *static_cast<double *>(ptr) = fval;
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
        {
          double fval = 0.0;

          if (!lwm2m_data_decode_float(&data, &fval))
            {
              return -EINVAL;
            }

          if (fval < static_cast<double>(std::numeric_limits<int32_t>::min()) / 65536.0 ||
              fval > static_cast<double>(std::numeric_limits<int32_t>::max()) / 65536.0)
            {
              return -ERANGE;
            }

          *static_cast<b16_t *>(ptr) = ftob16(fval);
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UB16
      case SObjectId::DTYPE_UB16:
        {
          double fval = 0.0;

          if (!lwm2m_data_decode_float(&data, &fval))
            {
              return -EINVAL;
            }

          if (fval < 0.0 ||
              fval > static_cast<double>(std::numeric_limits<uint32_t>::max()) / 65536.0)
            {
              return -ERANGE;
            }

          *static_cast<ub16_t *>(ptr) = static_cast<ub16_t>(fval * 65536.0);
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_BLOCK
      case SObjectId::DTYPE_BLOCK:
        {
          const uint8_t *buffer;
          size_t size;
          const size_t capacity = res.data->getBufferSize();

          if (!bufferValue(data, &buffer, &size))
            {
              return -EINVAL;
            }

          if (size > capacity)
            {
              return -EFBIG;
            }

          res.data->N = size;
          if (size > 0)
            {
              std::memcpy(res.data->getDataPtr(), buffer, size);
            }

          return io->setData(*res.data);
        }
#endif

      default:
        return -ENOTSUP;
    }

  return io->setData(*res.data);
}

int ObjectBinding::executeResource(Resource &res, const uint8_t *buffer, int length)
{
  CIOCommon *io;
  uint8_t cmd = CObject::CMD_RESET;
  int ret;

  if (proto == nullptr)
    {
      return -ENODEV;
    }

  io = res.io;
  if (io == nullptr)
    {
      return -ENOENT;
    }

  if (length > 0)
    {
      if (buffer == nullptr || length != 1 || buffer[0] < '0' || buffer[0] > '3')
        {
          return -EINVAL;
        }

      cmd = static_cast<uint8_t>(buffer[0] - '0');
    }

  ret = io->trigger(cmd);
  if (ret != -ENOTSUP)
    {
      return ret;
    }

  if (res.data == nullptr || !io->isWrite())
    {
      return -ENOTSUP;
    }

#ifdef CONFIG_DAWN_DTYPE_UINT8
  if (io->getDtype() == SObjectId::DTYPE_UINT8)
    {
      *static_cast<uint8_t *>(res.data->getDataPtr()) = cmd;
      return io->setData(*res.data);
    }
#endif

  return -ENOTSUP;
}

uint8_t ObjectBinding::readCb(lwm2m_context_t *ctx,
                              uint16_t instanceId,
                              int *numData,
                              lwm2m_data_t **dataArray,
                              lwm2m_object_t *object)
{
  ObjectBinding *self = static_cast<ObjectBinding *>(object->userData);

  UNUSED(ctx);

  if (self == nullptr)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

  if (*numData == 0)
    {
      Instance *inst =
        reinterpret_cast<Instance *>(lwm2m_list_find(object->instanceList, instanceId));
      size_t idx = 0;
      size_t count = 0;

      if (inst == nullptr)
        {
          return COAP_404_NOT_FOUND;
        }

      for (const Resource &res : self->resources)
        {
          if (res.instanceId == instanceId &&
              (res.access & CProtoWakaama::PROTO_WAKAAMA_ACCESS_READ) != 0)
            {
              count++;
            }
        }

      *numData = static_cast<int>(count);
      *dataArray = *numData > 0 ? lwm2m_data_new(*numData) : nullptr;
      if (*numData > 0 && *dataArray == nullptr)
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }

      for (const Resource &res : self->resources)
        {
          if (res.instanceId == instanceId &&
              (res.access & CProtoWakaama::PROTO_WAKAAMA_ACCESS_READ) != 0)
            {
              (*dataArray)[idx++].id = res.resourceId;
            }
        }
    }

  for (int i = 0; i < *numData; i++)
    {
      Resource *res = self->findResource(instanceId, (*dataArray)[i].id);
      if (res == nullptr || (res->access & CProtoWakaama::PROTO_WAKAAMA_ACCESS_READ) == 0)
        {
          return COAP_404_NOT_FOUND;
        }

      if (self->readResource(*res, (*dataArray)[i]) < 0)
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

  return COAP_205_CONTENT;
}

uint8_t ObjectBinding::discoverCb(lwm2m_context_t *ctx,
                                  uint16_t instanceId,
                                  int *numData,
                                  lwm2m_data_t **dataArray,
                                  lwm2m_object_t *object)
{
  ObjectBinding *self = static_cast<ObjectBinding *>(object->userData);

  UNUSED(ctx);

  if (self == nullptr)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

  if (*numData == 0)
    {
      Instance *inst =
        reinterpret_cast<Instance *>(lwm2m_list_find(object->instanceList, instanceId));
      size_t idx = 0;

      if (inst == nullptr)
        {
          return COAP_404_NOT_FOUND;
        }

      *numData = static_cast<int>(inst->resourceCount);
      *dataArray = *numData > 0 ? lwm2m_data_new(*numData) : nullptr;
      if (*numData > 0 && *dataArray == nullptr)
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }

      for (const Resource &res : self->resources)
        {
          if (res.instanceId == instanceId)
            {
              (*dataArray)[idx++].id = res.resourceId;
            }
        }
    }

  return COAP_205_CONTENT;
}

uint8_t ObjectBinding::writeCb(lwm2m_context_t *ctx,
                               uint16_t instanceId,
                               int numData,
                               lwm2m_data_t *dataArray,
                               lwm2m_object_t *object,
                               lwm2m_write_type_t writeType)
{
  ObjectBinding *self = static_cast<ObjectBinding *>(object->userData);

  UNUSED(ctx);
  UNUSED(writeType);

  if (self == nullptr)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

  for (int i = 0; i < numData; i++)
    {
      Resource *res = self->findResource(instanceId, dataArray[i].id);
      int ret;

      if (res == nullptr)
        {
          return COAP_404_NOT_FOUND;
        }

      if ((res->access & CProtoWakaama::PROTO_WAKAAMA_ACCESS_WRITE) == 0)
        {
          return COAP_405_METHOD_NOT_ALLOWED;
        }

      ret = self->writeResource(*res, dataArray[i]);
      if (ret == -EINVAL || ret == -ERANGE)
        {
          return COAP_400_BAD_REQUEST;
        }

      if (ret == -EFBIG)
        {
          return COAP_413_ENTITY_TOO_LARGE;
        }

      if (ret < 0)
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }

#ifdef CONFIG_DAWN_IO_NOTIFY
      if ((res->access & CProtoWakaama::PROTO_WAKAAMA_ACCESS_READ) != 0 && self->proto != nullptr)
        {
          self->proto->queueResourceChanged(self->lwm2m.objID, instanceId, res->resourceId);
        }
#endif
    }

  return COAP_204_CHANGED;
}

uint8_t ObjectBinding::executeCb(lwm2m_context_t *ctx,
                                 uint16_t instanceId,
                                 uint16_t resourceId,
                                 uint8_t *buffer,
                                 int length,
                                 lwm2m_object_t *object)
{
  ObjectBinding *self = static_cast<ObjectBinding *>(object->userData);
  Resource *res;
  int ret;

  UNUSED(ctx);

  if (self == nullptr)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

  res = self->findResource(instanceId, resourceId);
  if (res == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  if ((res->access & CProtoWakaama::PROTO_WAKAAMA_ACCESS_EXECUTE) == 0)
    {
      return COAP_405_METHOD_NOT_ALLOWED;
    }

  ret = self->executeResource(*res, buffer, length);
  if (ret == -EINVAL || ret == -ERANGE)
    {
      return COAP_400_BAD_REQUEST;
    }

  if (ret == -ENOENT || ret == -ENOTSUP)
    {
      return COAP_405_METHOD_NOT_ALLOWED;
    }

  if (ret < 0)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

  return COAP_204_CHANGED;
}
