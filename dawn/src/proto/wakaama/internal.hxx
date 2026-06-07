// dawn/src/proto/wakaama/internal.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/proto/wakaama/wakaama.hxx"

extern "C"
{
#include <liblwm2m.h>
}

#include <cstddef>
#include <cstdint>

namespace dawn
{
namespace wakaama_internal
{
/** @brief Maximum bytes reserved for a Security object URI. */

constexpr size_t WAKAAMA_SECURITY_URI_CAP = CProtoWakaama::RX_BUFFER_SIZE;

/** @brief Maximum bytes reserved for Security object credentials. */

constexpr size_t WAKAAMA_SECURITY_CREDENTIAL_CAP = CProtoWakaama::RX_BUFFER_SIZE;

/** @brief Magic marker for extended server descriptor payloads. */

constexpr uint32_t WAKAAMA_SERVER_EXT_MAGIC = 0x574b4131; // WKA1

/** @brief Server descriptor flag selecting the coaps URI scheme. */

constexpr uint32_t WAKAAMA_SERVER_FLAG_COAPS = (1 << 0);

/** @brief Server descriptor flag marking a bootstrap server instance. */

constexpr uint32_t WAKAAMA_SERVER_FLAG_BOOTSTRAP = (1 << 16);

/** @brief Bit offset of the server descriptor security-mode field. */

constexpr unsigned int WAKAAMA_SERVER_FLAG_SECURITY_SHIFT = 8;

/** @brief Mask for the server descriptor security-mode field. */

constexpr unsigned int WAKAAMA_SERVER_FLAG_SECURITY_MASK = 0xff;

/** @brief Runtime representation of one LwM2M Security object instance. */

struct security_instance_s
{
  security_instance_s *next;
  uint16_t id;
  const char *uri;
  const uint8_t *publicIdentity;
  size_t publicIdentityLen;
  const uint8_t *secretKey;
  size_t secretKeyLen;
  uint16_t shortServerId;
  uint32_t holdoff;
  uint32_t bootstrapTimeout;
  uint8_t securityMode;
  bool bootstrap;
  bool allocated;
  char uriBuffer[WAKAAMA_SECURITY_URI_CAP];
  uint8_t publicIdentityBuffer[WAKAAMA_SECURITY_CREDENTIAL_CAP];
  uint8_t secretKeyBuffer[WAKAAMA_SECURITY_CREDENTIAL_CAP];
};

/** @brief Runtime representation of one LwM2M Server object instance. */

struct server_instance_s
{
  server_instance_s *next;
  uint16_t id;
  uint16_t shortServerId;
  uint32_t lifetime;
  uint32_t defaultMinPeriod;
  uint32_t defaultMaxPeriod;
  uint32_t disableTimeout;
  uint32_t initialRegistrationDelay;
  uint32_t communicationRetryTimer;
  uint32_t communicationSequenceDelayTimer;
  uint16_t registrationOrder;
  uint8_t communicationRetryCount;
  uint8_t communicationSequenceRetryCount;
  bool storing;
  bool registrationFailureBlock;
  bool bootstrapOnRegistrationFailure;
  bool muteSend;
  bool allocated;
  char binding[4];
  char preferredTransport[4];
};

/** @brief Runtime representation of the single LwM2M Device instance. */

struct device_instance_s
{
  device_instance_s *next;
  uint16_t id;
};

/** @brief Fixed pools backing bootstrap-mutable Security and Server objects. */

struct InstancePools
{
  security_instance_s *security;
  server_instance_s *server;
  size_t securityCapacity;
  size_t serverCapacity;
};

/** @brief Allocate an unused Security instance from the fixed pool. */

security_instance_s *allocateSecurityInstance(InstancePools *pools);

/** @brief Release a Security instance back to the fixed pool. */

void releaseSecurityInstance(security_instance_s *inst);

/** @brief Allocate an unused Server instance from the fixed pool. */

server_instance_s *allocateServerInstance(InstancePools *pools);

/** @brief Release a Server instance back to the fixed pool. */

void releaseServerInstance(server_instance_s *inst);

/** @brief Copy a URI value into a Security instance string buffer. */

bool assignSecurityString(security_instance_s &inst, const uint8_t *src, size_t len);

/** @brief Copy credential bytes into a fixed Security instance buffer. */

bool assignSecurityBuffer(const uint8_t *src,
                          size_t len,
                          uint8_t *buffer,
                          size_t capacity,
                          const uint8_t **dst,
                          size_t *dstLen);

/** @brief Read callback for Wakaama Security object resources. */

uint8_t securityRead(lwm2m_context_t *ctx,
                     uint16_t instanceId,
                     int *numData,
                     lwm2m_data_t **dataArray,
                     lwm2m_object_t *object);

/** @brief Discover callback for Wakaama Security object resources. */

uint8_t securityDiscover(lwm2m_context_t *ctx,
                         uint16_t instanceId,
                         int *numData,
                         lwm2m_data_t **dataArray,
                         lwm2m_object_t *object);
#ifdef CONFIG_WAKAAMA_BOOTSTRAP
/** @brief Bootstrap write callback for Wakaama Security object resources. */

uint8_t securityWrite(lwm2m_context_t *ctx,
                      uint16_t instanceId,
                      int numData,
                      lwm2m_data_t *dataArray,
                      lwm2m_object_t *object,
                      lwm2m_write_type_t writeType);

/** @brief Bootstrap create callback for Wakaama Security object instances. */

uint8_t securityCreate(lwm2m_context_t *ctx,
                       uint16_t instanceId,
                       int numData,
                       lwm2m_data_t *dataArray,
                       lwm2m_object_t *object);

/** @brief Bootstrap delete callback for Wakaama Security object instances. */

uint8_t securityDelete(lwm2m_context_t *ctx, uint16_t instanceId, lwm2m_object_t *object);
#endif

/** @brief Read callback for Wakaama Server object resources. */

uint8_t serverRead(lwm2m_context_t *ctx,
                   uint16_t instanceId,
                   int *numData,
                   lwm2m_data_t **dataArray,
                   lwm2m_object_t *object);

/** @brief Discover callback for Wakaama Server object resources. */

uint8_t serverDiscover(lwm2m_context_t *ctx,
                       uint16_t instanceId,
                       int *numData,
                       lwm2m_data_t **dataArray,
                       lwm2m_object_t *object);

/** @brief Execute callback for Wakaama Server object resources. */

uint8_t serverExecute(lwm2m_context_t *ctx,
                      uint16_t instanceId,
                      uint16_t resourceId,
                      uint8_t *buffer,
                      int length,
                      lwm2m_object_t *object);

/** @brief Write callback for Wakaama Server object resources. */

uint8_t serverWrite(lwm2m_context_t *ctx,
                    uint16_t instanceId,
                    int numData,
                    lwm2m_data_t *dataArray,
                    lwm2m_object_t *object,
                    lwm2m_write_type_t writeType);
#ifdef CONFIG_WAKAAMA_BOOTSTRAP
/** @brief Bootstrap create callback for Wakaama Server object instances. */

uint8_t serverCreate(lwm2m_context_t *ctx,
                     uint16_t instanceId,
                     int numData,
                     lwm2m_data_t *dataArray,
                     lwm2m_object_t *object);

/** @brief Bootstrap delete callback for Wakaama Server object instances. */

uint8_t serverDelete(lwm2m_context_t *ctx, uint16_t instanceId, lwm2m_object_t *object);
#endif

/** @brief Read callback for Wakaama Device object resources. */

uint8_t deviceRead(lwm2m_context_t *ctx,
                   uint16_t instanceId,
                   int *numData,
                   lwm2m_data_t **dataArray,
                   lwm2m_object_t *object);

/** @brief Discover callback for Wakaama Device object resources. */

uint8_t deviceDiscover(lwm2m_context_t *ctx,
                       uint16_t instanceId,
                       int *numData,
                       lwm2m_data_t **dataArray,
                       lwm2m_object_t *object);

/** @brief Resolve the Device object battery IO bindings (device: config) to
 *  their IO objects and allocate read buffers. Called once at build time.
 */

void deviceResolveBatteryBindings(CProtoWakaama *proto);

} // namespace wakaama_internal
} // namespace dawn
