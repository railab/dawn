// dawn/src/proto/wakaama/client.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "internal.hxx"
#include "object_binding.hxx"

#include <ctime>
#include <vector>

namespace dawn
{
namespace wakaama_internal
{
/**
 * @brief Wakaama client context and object registry runtime.
 */

class ClientRuntime
{
public:
  /** @brief Construct runtime state for a Wakaama protocol owner. */

  explicit ClientRuntime(CProtoWakaama &owner);

  /** @brief Close the Wakaama context and release owned object allocations. */

  ~ClientRuntime();

  /** @brief Build built-in and descriptor-backed LwM2M objects. */

  int build(const std::vector<ObjectBinding *> &objects);

  /** @brief Release built-in and descriptor-backed LwM2M objects. */

  void destroy(const std::vector<ObjectBinding *> &objects);

  /** @brief Open the Wakaama client context with transport callback userdata. */

  int openContext(void *userdata);

  /** @brief Configure the Wakaama client endpoint and registered objects. */

  int configure(const char *endpoint);

  /** @brief Run one Wakaama state-machine step. */

  int step(time_t *timeout);

  /** @brief Return true when the Wakaama client has completed registration. */

  bool ready() const;

  /** @brief Deliver one received packet into the Wakaama client context. */

  void handlePacket(uint8_t *buffer, size_t length, void *session);

  /** @brief Find a Security object instance by instance ID. */

  security_instance_s *findSecurityInstance(uint16_t securityInstanceId) const;

  /** @brief Return the underlying Wakaama client context. */

  lwm2m_context_t *context() const
  {
    return ctx;
  }

private:
  CProtoWakaama &owner;
  lwm2m_context_t *ctx;
  lwm2m_object_t *securityObj;
  lwm2m_object_t *serverObj;
  lwm2m_object_t *deviceObj;
  InstancePools *instancePools;
  std::vector<lwm2m_object_t *> lwm2mObjects;

  /** @brief Build built-in Security and Server objects from server config. */

  int buildSecurityAndServerObjects();

  /** @brief Build the built-in Device object. */

  int buildDeviceObject();

  /** @brief Close the Wakaama client context if it is open. */

  void closeContext();
};

} // namespace wakaama_internal
} // namespace dawn
