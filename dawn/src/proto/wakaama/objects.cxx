// dawn/src/proto/wakaama/objects.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "internal.hxx"

#include <cstring>
#include <limits>

using namespace dawn;
using namespace dawn::wakaama_internal;

namespace
{
constexpr uint32_t SERVER_DISABLE_TIMEOUT_DEFAULT = 86400;
constexpr uint32_t SERVER_COMMUNICATION_RETRY_TIMER_DEFAULT = 60;
constexpr uint32_t SERVER_COMMUNICATION_SEQUENCE_DELAY_TIMER_DEFAULT = 86400;
constexpr uint8_t SERVER_COMMUNICATION_RETRY_COUNT_DEFAULT = 5;
constexpr uint8_t SERVER_COMMUNICATION_SEQUENCE_RETRY_COUNT_DEFAULT = 1;

constexpr uint16_t SECURITY_RESOURCE_IDS[] = {LWM2M_SECURITY_URI_ID,
                                              LWM2M_SECURITY_BOOTSTRAP_ID,
                                              LWM2M_SECURITY_SECURITY_ID,
                                              LWM2M_SECURITY_PUBLIC_KEY_ID,
                                              LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID,
                                              LWM2M_SECURITY_SECRET_KEY_ID,
                                              LWM2M_SECURITY_SHORT_SERVER_ID,
                                              LWM2M_SECURITY_HOLD_OFF_ID,
                                              LWM2M_SECURITY_BOOTSTRAP_TIMEOUT_ID};

constexpr uint16_t SERVER_READ_RESOURCE_IDS[] = {LWM2M_SERVER_SHORT_ID_ID,
                                                 LWM2M_SERVER_LIFETIME_ID,
                                                 LWM2M_SERVER_MIN_PERIOD_ID,
                                                 LWM2M_SERVER_MAX_PERIOD_ID,
                                                 LWM2M_SERVER_TIMEOUT_ID,
                                                 LWM2M_SERVER_STORING_ID,
                                                 LWM2M_SERVER_BINDING_ID,
                                                 LWM2M_SERVER_REG_ORDER_ID,
                                                 LWM2M_SERVER_INITIAL_REG_DELAY_ID,
                                                 LWM2M_SERVER_REG_FAIL_BLOCK_ID,
                                                 LWM2M_SERVER_REG_FAIL_BOOTSTRAP_ID,
                                                 LWM2M_SERVER_COMM_RETRY_COUNT_ID,
                                                 LWM2M_SERVER_COMM_RETRY_TIMER_ID,
                                                 LWM2M_SERVER_SEQ_DELAY_TIMER_ID,
                                                 LWM2M_SERVER_SEQ_RETRY_COUNT_ID,
                                                 LWM2M_SERVER_PREFERRED_TRANSPORT_ID,
                                                 LWM2M_SERVER_MUTE_SEND_ID};

constexpr uint16_t SERVER_DISCOVER_RESOURCE_IDS[] = {LWM2M_SERVER_SHORT_ID_ID,
                                                     LWM2M_SERVER_LIFETIME_ID,
                                                     LWM2M_SERVER_MIN_PERIOD_ID,
                                                     LWM2M_SERVER_MAX_PERIOD_ID,
                                                     LWM2M_SERVER_DISABLE_ID,
                                                     LWM2M_SERVER_TIMEOUT_ID,
                                                     LWM2M_SERVER_STORING_ID,
                                                     LWM2M_SERVER_BINDING_ID,
                                                     LWM2M_SERVER_UPDATE_ID,
                                                     LWM2M_SERVER_REG_ORDER_ID,
                                                     LWM2M_SERVER_INITIAL_REG_DELAY_ID,
                                                     LWM2M_SERVER_REG_FAIL_BLOCK_ID,
                                                     LWM2M_SERVER_REG_FAIL_BOOTSTRAP_ID,
                                                     LWM2M_SERVER_COMM_RETRY_COUNT_ID,
                                                     LWM2M_SERVER_COMM_RETRY_TIMER_ID,
                                                     LWM2M_SERVER_SEQ_DELAY_TIMER_ID,
                                                     LWM2M_SERVER_SEQ_RETRY_COUNT_ID,
                                                     LWM2M_SERVER_PREFERRED_TRANSPORT_ID,
                                                     LWM2M_SERVER_MUTE_SEND_ID};

constexpr uint16_t DEVICE_RESOURCE_IDS[] = {
  CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_MANUFACTURER,
  CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_MODEL_NUMBER,
  CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_SERIAL_NUMBER,
  CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_FIRMWARE_VERSION,
  CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_ERROR_CODE,
  CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_CURRENT_TIME,
  CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_BINDING_MODES,
};

bool encodeString(const char *str, lwm2m_data_t *data)
{
  lwm2m_data_encode_string(str == nullptr ? "" : str, data);
  return data->type == LWM2M_TYPE_STRING;
}

bool encodeOpaque(const uint8_t *buffer, size_t len, lwm2m_data_t *data)
{
  lwm2m_data_encode_opaque(buffer, len, data);
  return data->type == LWM2M_TYPE_OPAQUE;
}

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

bool allocateResourceList(const uint16_t *ids, int count, int *numData, lwm2m_data_t **dataArray)
{
  *dataArray = lwm2m_data_new(count);
  if (*dataArray == nullptr)
    {
      return false;
    }

  *numData = count;
  for (int i = 0; i < count; i++)
    {
      (*dataArray)[i].id = ids[i];
    }

  return true;
}

template<typename T>
bool decodeUint(lwm2m_data_t *data, T *value)
{
  int64_t intValue;

  if (lwm2m_data_decode_int(data, &intValue) != 1 || intValue < 0 ||
      static_cast<uint64_t>(intValue) > static_cast<uint64_t>(std::numeric_limits<T>::max()))
    {
      return false;
    }

  *value = static_cast<T>(intValue);
  return true;
}

bool assignShortString(char *dest, size_t capacity, const lwm2m_data_t &data)
{
  if ((data.type != LWM2M_TYPE_STRING && data.type != LWM2M_TYPE_OPAQUE) ||
      data.value.asBuffer.length == 0 || data.value.asBuffer.length >= capacity ||
      data.value.asBuffer.buffer == nullptr)
    {
      return false;
    }

  std::memcpy(dest, data.value.asBuffer.buffer, data.value.asBuffer.length);
  dest[data.value.asBuffer.length] = '\0';
  return true;
}

void initServerDefaults(server_instance_s &inst)
{
  inst.binding[0] = 'U';
  inst.binding[1] = '\0';
  inst.preferredTransport[0] = 'U';
  inst.preferredTransport[1] = '\0';
  inst.disableTimeout = SERVER_DISABLE_TIMEOUT_DEFAULT;
  inst.bootstrapOnRegistrationFailure = true;
  inst.communicationRetryCount = SERVER_COMMUNICATION_RETRY_COUNT_DEFAULT;
  inst.communicationRetryTimer = SERVER_COMMUNICATION_RETRY_TIMER_DEFAULT;
  inst.communicationSequenceDelayTimer = SERVER_COMMUNICATION_SEQUENCE_DELAY_TIMER_DEFAULT;
  inst.communicationSequenceRetryCount = SERVER_COMMUNICATION_SEQUENCE_RETRY_COUNT_DEFAULT;
}
} // namespace

security_instance_s *dawn::wakaama_internal::allocateSecurityInstance(InstancePools *pools)
{
  if (pools == nullptr)
    {
      return nullptr;
    }

  for (size_t i = 0; i < pools->securityCapacity; i++)
    {
      security_instance_s *inst = &pools->security[i];

      if (!inst->allocated)
        {
          std::memset(inst, 0, sizeof(*inst));
          inst->allocated = true;
          inst->uri = inst->uriBuffer;
          return inst;
        }
    }

  return nullptr;
}

void dawn::wakaama_internal::releaseSecurityInstance(security_instance_s *inst)
{
  if (inst != nullptr)
    {
      std::memset(inst, 0, sizeof(*inst));
    }
}

server_instance_s *dawn::wakaama_internal::allocateServerInstance(InstancePools *pools)
{
  if (pools == nullptr)
    {
      return nullptr;
    }

  for (size_t i = 0; i < pools->serverCapacity; i++)
    {
      server_instance_s *inst = &pools->server[i];

      if (!inst->allocated)
        {
          std::memset(inst, 0, sizeof(*inst));
          inst->allocated = true;
          initServerDefaults(*inst);
          return inst;
        }
    }

  return nullptr;
}

void dawn::wakaama_internal::releaseServerInstance(server_instance_s *inst)
{
  if (inst != nullptr)
    {
      std::memset(inst, 0, sizeof(*inst));
    }
}

bool dawn::wakaama_internal::assignSecurityString(security_instance_s &inst,
                                                  const uint8_t *src,
                                                  size_t len)
{
  if (len > 0 && src == nullptr)
    {
      return false;
    }

  while (len > 0 && src[len - 1] == '\0')
    {
      len--;
    }

  if (len >= sizeof(inst.uriBuffer))
    {
      return false;
    }

  if (len > 0)
    {
      std::memcpy(inst.uriBuffer, src, len);
    }

  inst.uriBuffer[len] = '\0';
  inst.uri = inst.uriBuffer;
  return true;
}

bool dawn::wakaama_internal::assignSecurityBuffer(const uint8_t *src,
                                                  size_t len,
                                                  uint8_t *buffer,
                                                  size_t capacity,
                                                  const uint8_t **dst,
                                                  size_t *dstLen)
{
  if (len > capacity)
    {
      return false;
    }

  if (len > 0 && src == nullptr)
    {
      return false;
    }

  if (len > 0)
    {
      std::memcpy(buffer, src, len);
      *dst = buffer;
    }
  else
    {
      *dst = nullptr;
    }

  *dstLen = len;
  return true;
}

uint8_t dawn::wakaama_internal::securityRead(lwm2m_context_t *ctx,
                                             uint16_t instanceId,
                                             int *numData,
                                             lwm2m_data_t **dataArray,
                                             lwm2m_object_t *object)
{
  security_instance_s *inst;

  UNUSED(ctx);

  inst = reinterpret_cast<security_instance_s *>(lwm2m_list_find(object->instanceList, instanceId));
  if (inst == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  if (*numData == 0)
    {
      if (!allocateResourceList(
            SECURITY_RESOURCE_IDS,
            static_cast<int>(sizeof(SECURITY_RESOURCE_IDS) / sizeof(SECURITY_RESOURCE_IDS[0])),
            numData,
            dataArray))
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

  for (int i = 0; i < *numData; i++)
    {
      switch ((*dataArray)[i].id)
        {
          case LWM2M_SECURITY_URI_ID:
            if (!encodeString(inst->uri, &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case LWM2M_SECURITY_BOOTSTRAP_ID:
            lwm2m_data_encode_bool(inst->bootstrap, &(*dataArray)[i]);
            break;

          case LWM2M_SECURITY_SECURITY_ID:
            lwm2m_data_encode_int(inst->securityMode, &(*dataArray)[i]);
            break;

          case LWM2M_SECURITY_PUBLIC_KEY_ID:
            if (!encodeOpaque(inst->publicIdentity, inst->publicIdentityLen, &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID:
            if (!encodeOpaque(nullptr, 0, &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case LWM2M_SECURITY_SECRET_KEY_ID:
            if (!encodeOpaque(inst->secretKey, inst->secretKeyLen, &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case LWM2M_SECURITY_SHORT_SERVER_ID:
            lwm2m_data_encode_int(inst->shortServerId, &(*dataArray)[i]);
            break;

          case LWM2M_SECURITY_HOLD_OFF_ID:
            lwm2m_data_encode_int(inst->holdoff, &(*dataArray)[i]);
            break;

          case LWM2M_SECURITY_BOOTSTRAP_TIMEOUT_ID:
            lwm2m_data_encode_int(inst->bootstrapTimeout, &(*dataArray)[i]);
            break;

          default:
            return COAP_404_NOT_FOUND;
        }
    }

  return COAP_205_CONTENT;
}

uint8_t dawn::wakaama_internal::securityDiscover(lwm2m_context_t *ctx,
                                                 uint16_t instanceId,
                                                 int *numData,
                                                 lwm2m_data_t **dataArray,
                                                 lwm2m_object_t *object)
{
  security_instance_s *inst;

  UNUSED(ctx);

  inst = reinterpret_cast<security_instance_s *>(lwm2m_list_find(object->instanceList, instanceId));
  if (inst == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  if (*numData == 0)
    {
      if (!allocateResourceList(
            SECURITY_RESOURCE_IDS,
            static_cast<int>(sizeof(SECURITY_RESOURCE_IDS) / sizeof(SECURITY_RESOURCE_IDS[0])),
            numData,
            dataArray))
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

  return COAP_205_CONTENT;
}

#ifdef CONFIG_WAKAAMA_BOOTSTRAP
uint8_t dawn::wakaama_internal::securityWrite(lwm2m_context_t *ctx,
                                              uint16_t instanceId,
                                              int numData,
                                              lwm2m_data_t *dataArray,
                                              lwm2m_object_t *object,
                                              lwm2m_write_type_t writeType)
{
  security_instance_s *inst;

  UNUSED(ctx);
  UNUSED(writeType);

  inst = reinterpret_cast<security_instance_s *>(lwm2m_list_find(object->instanceList, instanceId));
  if (inst == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  for (int i = 0; i < numData; i++)
    {
      int64_t intValue;
      bool boolValue;

      if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
          return COAP_404_NOT_FOUND;
        }

      switch (dataArray[i].id)
        {
          case LWM2M_SECURITY_URI_ID:
            {
              const uint8_t *buffer;
              size_t length;

              if (!bufferValue(dataArray[i], &buffer, &length))
                {
                  return COAP_400_BAD_REQUEST;
                }

              if (!assignSecurityString(*inst, buffer, length))
                {
                  return COAP_500_INTERNAL_SERVER_ERROR;
                }
            }
            break;

          case LWM2M_SECURITY_BOOTSTRAP_ID:
            if (lwm2m_data_decode_bool(dataArray + i, &boolValue) != 1)
              {
                return COAP_400_BAD_REQUEST;
              }

            inst->bootstrap = boolValue;
            break;

          case LWM2M_SECURITY_SECURITY_ID:
            if (lwm2m_data_decode_int(dataArray + i, &intValue) != 1 || intValue < 0 ||
                intValue > 3)
              {
                return COAP_400_BAD_REQUEST;
              }

            inst->securityMode = static_cast<uint8_t>(intValue);
            break;

          case LWM2M_SECURITY_PUBLIC_KEY_ID:
            {
              const uint8_t *buffer;
              size_t length;

              if (!bufferValue(dataArray[i], &buffer, &length))
                {
                  return COAP_400_BAD_REQUEST;
                }

              if (!assignSecurityBuffer(buffer,
                                        length,
                                        inst->publicIdentityBuffer,
                                        sizeof(inst->publicIdentityBuffer),
                                        &inst->publicIdentity,
                                        &inst->publicIdentityLen))
                {
                  return COAP_500_INTERNAL_SERVER_ERROR;
                }
            }
            break;

          case LWM2M_SECURITY_SECRET_KEY_ID:
            {
              const uint8_t *buffer;
              size_t length;

              if (!bufferValue(dataArray[i], &buffer, &length))
                {
                  return COAP_400_BAD_REQUEST;
                }

              if (!assignSecurityBuffer(buffer,
                                        length,
                                        inst->secretKeyBuffer,
                                        sizeof(inst->secretKeyBuffer),
                                        &inst->secretKey,
                                        &inst->secretKeyLen))
                {
                  return COAP_500_INTERNAL_SERVER_ERROR;
                }
            }
            break;

          case LWM2M_SECURITY_SERVER_PUBLIC_KEY_ID:
            break;

          case LWM2M_SECURITY_SHORT_SERVER_ID:
            if (lwm2m_data_decode_int(dataArray + i, &intValue) != 1 || intValue < 0 ||
                intValue > UINT16_MAX)
              {
                return COAP_400_BAD_REQUEST;
              }

            inst->shortServerId = static_cast<uint16_t>(intValue);
            break;

          case LWM2M_SECURITY_HOLD_OFF_ID:
            if (lwm2m_data_decode_int(dataArray + i, &intValue) != 1 || intValue < 0 ||
                intValue > UINT32_MAX)
              {
                return COAP_400_BAD_REQUEST;
              }

            inst->holdoff = static_cast<uint32_t>(intValue);
            break;

          case LWM2M_SECURITY_BOOTSTRAP_TIMEOUT_ID:
            if (lwm2m_data_decode_int(dataArray + i, &intValue) != 1 || intValue < 0 ||
                intValue > UINT32_MAX)
              {
                return COAP_400_BAD_REQUEST;
              }

            inst->bootstrapTimeout = static_cast<uint32_t>(intValue);
            break;

          default:
            return COAP_400_BAD_REQUEST;
        }
    }

  return COAP_204_CHANGED;
}

uint8_t dawn::wakaama_internal::securityCreate(lwm2m_context_t *ctx,
                                               uint16_t instanceId,
                                               int numData,
                                               lwm2m_data_t *dataArray,
                                               lwm2m_object_t *object)
{
  InstancePools *pools = static_cast<InstancePools *>(object->userData);
  security_instance_s *inst;
  uint8_t result;

  if (lwm2m_list_find(object->instanceList, instanceId) != nullptr)
    {
      return COAP_400_BAD_REQUEST;
    }

  inst = allocateSecurityInstance(pools);
  if (inst == nullptr)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

  inst->id = instanceId;
  inst->securityMode = LWM2M_SECURITY_MODE_NONE;
  inst->holdoff = 10;
  object->instanceList = LWM2M_LIST_ADD(object->instanceList, inst);

  result =
    securityWrite(ctx, instanceId, numData, dataArray, object, LWM2M_WRITE_REPLACE_RESOURCES);
  if (result != COAP_204_CHANGED)
    {
      lwm2m_list_t *removed;

      object->instanceList = lwm2m_list_remove(object->instanceList, instanceId, &removed);
      UNUSED(removed);
      releaseSecurityInstance(inst);
      return result;
    }

  return COAP_201_CREATED;
}

uint8_t dawn::wakaama_internal::securityDelete(lwm2m_context_t *ctx,
                                               uint16_t instanceId,
                                               lwm2m_object_t *object)
{
  lwm2m_list_t *removed;

  UNUSED(ctx);

  object->instanceList = lwm2m_list_remove(object->instanceList, instanceId, &removed);
  if (removed == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  releaseSecurityInstance(reinterpret_cast<security_instance_s *>(removed));
  return COAP_202_DELETED;
}
#endif

uint8_t dawn::wakaama_internal::serverRead(lwm2m_context_t *ctx,
                                           uint16_t instanceId,
                                           int *numData,
                                           lwm2m_data_t **dataArray,
                                           lwm2m_object_t *object)
{
  server_instance_s *inst;

  UNUSED(ctx);

  inst = reinterpret_cast<server_instance_s *>(lwm2m_list_find(object->instanceList, instanceId));
  if (inst == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  if (*numData == 0)
    {
      if (!allocateResourceList(SERVER_READ_RESOURCE_IDS,
                                static_cast<int>(sizeof(SERVER_READ_RESOURCE_IDS) /
                                                 sizeof(SERVER_READ_RESOURCE_IDS[0])),
                                numData,
                                dataArray))
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

  for (int i = 0; i < *numData; i++)
    {
      switch ((*dataArray)[i].id)
        {
          case LWM2M_SERVER_SHORT_ID_ID:
            lwm2m_data_encode_int(inst->shortServerId, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_LIFETIME_ID:
            lwm2m_data_encode_int(inst->lifetime, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_MIN_PERIOD_ID:
            lwm2m_data_encode_int(inst->defaultMinPeriod, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_MAX_PERIOD_ID:
            lwm2m_data_encode_int(inst->defaultMaxPeriod, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_DISABLE_ID:
          case LWM2M_SERVER_UPDATE_ID:
          case LWM2M_SERVER_TRIGGER_ID:
            return COAP_405_METHOD_NOT_ALLOWED;

          case LWM2M_SERVER_TIMEOUT_ID:
            lwm2m_data_encode_int(inst->disableTimeout, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_STORING_ID:
            lwm2m_data_encode_bool(inst->storing, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_BINDING_ID:
            if (!encodeString(inst->binding, &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case LWM2M_SERVER_REG_ORDER_ID:
            lwm2m_data_encode_int(inst->registrationOrder, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_INITIAL_REG_DELAY_ID:
            lwm2m_data_encode_int(inst->initialRegistrationDelay, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_REG_FAIL_BLOCK_ID:
            lwm2m_data_encode_bool(inst->registrationFailureBlock, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_REG_FAIL_BOOTSTRAP_ID:
            lwm2m_data_encode_bool(inst->bootstrapOnRegistrationFailure, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_COMM_RETRY_COUNT_ID:
            lwm2m_data_encode_int(inst->communicationRetryCount, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_COMM_RETRY_TIMER_ID:
            lwm2m_data_encode_int(inst->communicationRetryTimer, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_SEQ_DELAY_TIMER_ID:
            lwm2m_data_encode_int(inst->communicationSequenceDelayTimer, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_SEQ_RETRY_COUNT_ID:
            lwm2m_data_encode_int(inst->communicationSequenceRetryCount, &(*dataArray)[i]);
            break;

          case LWM2M_SERVER_PREFERRED_TRANSPORT_ID:
            if (!encodeString(inst->preferredTransport, &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case LWM2M_SERVER_MUTE_SEND_ID:
            lwm2m_data_encode_bool(inst->muteSend, &(*dataArray)[i]);
            break;

          default:
            return COAP_404_NOT_FOUND;
        }
    }

  return COAP_205_CONTENT;
}

uint8_t dawn::wakaama_internal::serverDiscover(lwm2m_context_t *ctx,
                                               uint16_t instanceId,
                                               int *numData,
                                               lwm2m_data_t **dataArray,
                                               lwm2m_object_t *object)
{
  server_instance_s *inst;

  UNUSED(ctx);

  inst = reinterpret_cast<server_instance_s *>(lwm2m_list_find(object->instanceList, instanceId));
  if (inst == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  if (*numData == 0)
    {
      if (!allocateResourceList(SERVER_DISCOVER_RESOURCE_IDS,
                                static_cast<int>(sizeof(SERVER_DISCOVER_RESOURCE_IDS) /
                                                 sizeof(SERVER_DISCOVER_RESOURCE_IDS[0])),
                                numData,
                                dataArray))
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

  return COAP_205_CONTENT;
}

uint8_t dawn::wakaama_internal::serverExecute(lwm2m_context_t *ctx,
                                              uint16_t instanceId,
                                              uint16_t resourceId,
                                              uint8_t *buffer,
                                              int length,
                                              lwm2m_object_t *object)
{
  server_instance_s *inst;

  UNUSED(buffer);
  UNUSED(length);

  inst = reinterpret_cast<server_instance_s *>(lwm2m_list_find(object->instanceList, instanceId));
  if (inst == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  switch (resourceId)
    {
      case LWM2M_SERVER_UPDATE_ID:
        return lwm2m_update_registration(ctx, inst->shortServerId, true) == COAP_NO_ERROR
                 ? COAP_204_CHANGED
                 : COAP_400_BAD_REQUEST;

      case LWM2M_SERVER_DISABLE_ID:
      case LWM2M_SERVER_TRIGGER_ID:
        return COAP_405_METHOD_NOT_ALLOWED;

      default:
        return COAP_405_METHOD_NOT_ALLOWED;
    }
}

uint8_t dawn::wakaama_internal::serverWrite(lwm2m_context_t *ctx,
                                            uint16_t instanceId,
                                            int numData,
                                            lwm2m_data_t *dataArray,
                                            lwm2m_object_t *object,
                                            lwm2m_write_type_t writeType)
{
  server_instance_s *inst;

  UNUSED(ctx);
  UNUSED(writeType);

  inst = reinterpret_cast<server_instance_s *>(lwm2m_list_find(object->instanceList, instanceId));
  if (inst == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  for (int i = 0; i < numData; i++)
    {
      bool boolValue;

      if (dataArray[i].type == LWM2M_TYPE_MULTIPLE_RESOURCE)
        {
          return COAP_404_NOT_FOUND;
        }

      switch (dataArray[i].id)
        {
          case LWM2M_SERVER_SHORT_ID_ID:
            if (!decodeUint(dataArray + i, &inst->shortServerId))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_LIFETIME_ID:
            if (!decodeUint(dataArray + i, &inst->lifetime))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_MIN_PERIOD_ID:
            if (!decodeUint(dataArray + i, &inst->defaultMinPeriod))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_MAX_PERIOD_ID:
            if (!decodeUint(dataArray + i, &inst->defaultMaxPeriod))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_TIMEOUT_ID:
            if (!decodeUint(dataArray + i, &inst->disableTimeout))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_STORING_ID:
            if (lwm2m_data_decode_bool(dataArray + i, &boolValue) != 1)
              {
                return COAP_400_BAD_REQUEST;
              }

            inst->storing = boolValue;
            break;

          case LWM2M_SERVER_BINDING_ID:
            if (!assignShortString(inst->binding, sizeof(inst->binding), dataArray[i]))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_UPDATE_ID:
          case LWM2M_SERVER_DISABLE_ID:
          case LWM2M_SERVER_TRIGGER_ID:
            return COAP_405_METHOD_NOT_ALLOWED;

          case LWM2M_SERVER_REG_ORDER_ID:
            if (!decodeUint(dataArray + i, &inst->registrationOrder))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_INITIAL_REG_DELAY_ID:
            if (!decodeUint(dataArray + i, &inst->initialRegistrationDelay))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_REG_FAIL_BLOCK_ID:
            if (lwm2m_data_decode_bool(dataArray + i, &boolValue) != 1)
              {
                return COAP_400_BAD_REQUEST;
              }

            inst->registrationFailureBlock = boolValue;
            break;

          case LWM2M_SERVER_REG_FAIL_BOOTSTRAP_ID:
            if (lwm2m_data_decode_bool(dataArray + i, &boolValue) != 1)
              {
                return COAP_400_BAD_REQUEST;
              }

            inst->bootstrapOnRegistrationFailure = boolValue;
            break;

          case LWM2M_SERVER_COMM_RETRY_COUNT_ID:
            if (!decodeUint(dataArray + i, &inst->communicationRetryCount))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_COMM_RETRY_TIMER_ID:
            if (!decodeUint(dataArray + i, &inst->communicationRetryTimer))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_SEQ_DELAY_TIMER_ID:
            if (!decodeUint(dataArray + i, &inst->communicationSequenceDelayTimer))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_SEQ_RETRY_COUNT_ID:
            if (!decodeUint(dataArray + i, &inst->communicationSequenceRetryCount))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_PREFERRED_TRANSPORT_ID:
            if (!assignShortString(
                  inst->preferredTransport, sizeof(inst->preferredTransport), dataArray[i]))
              {
                return COAP_400_BAD_REQUEST;
              }
            break;

          case LWM2M_SERVER_MUTE_SEND_ID:
            if (lwm2m_data_decode_bool(dataArray + i, &boolValue) != 1)
              {
                return COAP_400_BAD_REQUEST;
              }

            inst->muteSend = boolValue;
            break;

          default:
            return COAP_400_BAD_REQUEST;
        }
    }

  return COAP_204_CHANGED;
}

#ifdef CONFIG_WAKAAMA_BOOTSTRAP
uint8_t dawn::wakaama_internal::serverCreate(lwm2m_context_t *ctx,
                                             uint16_t instanceId,
                                             int numData,
                                             lwm2m_data_t *dataArray,
                                             lwm2m_object_t *object)
{
  InstancePools *pools = static_cast<InstancePools *>(object->userData);
  server_instance_s *inst;
  uint8_t result;

  if (lwm2m_list_find(object->instanceList, instanceId) != nullptr)
    {
      return COAP_400_BAD_REQUEST;
    }

  inst = allocateServerInstance(pools);
  if (inst == nullptr)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

  inst->id = instanceId;
  inst->binding[0] = 'U';
  object->instanceList = LWM2M_LIST_ADD(object->instanceList, inst);

  result = serverWrite(ctx, instanceId, numData, dataArray, object, LWM2M_WRITE_REPLACE_RESOURCES);
  if (result != COAP_204_CHANGED)
    {
      lwm2m_list_t *removed;

      object->instanceList = lwm2m_list_remove(object->instanceList, instanceId, &removed);
      UNUSED(removed);
      releaseServerInstance(inst);
      return result;
    }

  return COAP_201_CREATED;
}

uint8_t dawn::wakaama_internal::serverDelete(lwm2m_context_t *ctx,
                                             uint16_t instanceId,
                                             lwm2m_object_t *object)
{
  lwm2m_list_t *removed;

  UNUSED(ctx);

  object->instanceList = lwm2m_list_remove(object->instanceList, instanceId, &removed);
  if (removed == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  releaseServerInstance(reinterpret_cast<server_instance_s *>(removed));
  return COAP_202_DELETED;
}
#endif

uint8_t dawn::wakaama_internal::deviceRead(lwm2m_context_t *ctx,
                                           uint16_t instanceId,
                                           int *numData,
                                           lwm2m_data_t **dataArray,
                                           lwm2m_object_t *object)
{
  CProtoWakaama *proto;
  device_instance_s *inst;

  UNUSED(ctx);

  proto = static_cast<CProtoWakaama *>(object->userData);
  if (proto == nullptr)
    {
      return COAP_500_INTERNAL_SERVER_ERROR;
    }

  inst = reinterpret_cast<device_instance_s *>(lwm2m_list_find(object->instanceList, instanceId));
  if (inst == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  if (*numData == 0)
    {
      if (!allocateResourceList(
            DEVICE_RESOURCE_IDS,
            static_cast<int>(sizeof(DEVICE_RESOURCE_IDS) / sizeof(DEVICE_RESOURCE_IDS[0])),
            numData,
            dataArray))
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

  for (int i = 0; i < *numData; i++)
    {
      switch ((*dataArray)[i].id)
        {
          case CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_MANUFACTURER:
            if (!encodeString(proto->deviceManufacturer(), &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_MODEL_NUMBER:
            if (!encodeString(proto->deviceModelNumber(), &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_SERIAL_NUMBER:
            if (!encodeString(proto->deviceSerialNumber(), &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_FIRMWARE_VERSION:
            if (!encodeString(proto->deviceFirmwareVersion(), &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          case CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_ERROR_CODE:
            lwm2m_data_encode_int(0, &(*dataArray)[i]);
            break;

          case CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_CURRENT_TIME:
            lwm2m_data_encode_int(lwm2m_gettime(), &(*dataArray)[i]);
            break;

          case CProtoWakaama::WAKAAMA_DEVICE_RESOURCE_BINDING_MODES:
            if (!encodeString("U", &(*dataArray)[i]))
              {
                return COAP_500_INTERNAL_SERVER_ERROR;
              }
            break;

          default:
            return COAP_404_NOT_FOUND;
        }
    }

  return COAP_205_CONTENT;
}

uint8_t dawn::wakaama_internal::deviceDiscover(lwm2m_context_t *ctx,
                                               uint16_t instanceId,
                                               int *numData,
                                               lwm2m_data_t **dataArray,
                                               lwm2m_object_t *object)
{
  UNUSED(ctx);

  if (lwm2m_list_find(object->instanceList, instanceId) == nullptr)
    {
      return COAP_404_NOT_FOUND;
    }

  if (*numData == 0)
    {
      device_instance_s *inst =
        reinterpret_cast<device_instance_s *>(lwm2m_list_find(object->instanceList, instanceId));

      if (inst == nullptr)
        {
          return COAP_404_NOT_FOUND;
        }

      if (!allocateResourceList(
            DEVICE_RESOURCE_IDS,
            static_cast<int>(sizeof(DEVICE_RESOURCE_IDS) / sizeof(DEVICE_RESOURCE_IDS[0])),
            numData,
            dataArray))
        {
          return COAP_500_INTERNAL_SERVER_ERROR;
        }
    }

  return COAP_205_CONTENT;
}
