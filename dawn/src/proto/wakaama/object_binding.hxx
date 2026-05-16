// dawn/src/proto/wakaama/object_binding.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "internal.hxx"

#include <vector>

#include "dawn/io/ddata.hxx"

namespace dawn
{
namespace wakaama_internal
{
/**
 * @brief Runtime binding between one LwM2M object and Dawn IO resources.
 */

class ObjectBinding
{
public:
  /** @brief Descriptor-backed mapping for one object instance resource. */

  struct Resource
  {
    uint16_t instanceId;
    uint16_t resourceId;
    uint16_t access;
    SObjectId::ObjectId objid;
    CIOCommon *io;
    io_ddata_t *data;
  };

#ifdef CONFIG_DAWN_IO_NOTIFY
  /** @brief Notification callback context for one observable resource. */

  struct NotifyContext
  {
    CProtoWakaama *proto;
    CIOCommon *io;
    uint16_t objectId;
    uint16_t instanceId;
    uint16_t resourceId;
  };
#endif

  /** @brief Wakaama instance node used in the object instance list. */

  struct Instance
  {
    Instance *next;
    uint16_t id;
    size_t resourceCount;
  };

  /** @brief Construct binding state from one protocol object descriptor item. */

  ObjectBinding(const SObjectCfg::SObjectCfgItem *item, CProtoWakaama *proto);

  /** @brief Destroy binding state and release allocated Wakaama resources. */

  ~ObjectBinding();

  /** @brief Return descriptor parsing status captured by the constructor. */

  int configureStatus() const
  {
    return configStatus;
  }

  /** @brief Resolve Dawn IOs and build the Wakaama object instance list. */

  int init();

  /** @brief Release runtime allocations owned by the binding. */

  int deinit();

  /** @brief Return the Wakaama object exposed by this binding. */

  lwm2m_object_t *object()
  {
    return &lwm2m;
  }

private:
  CProtoWakaama *proto;
  int configStatus;
  lwm2m_object_t lwm2m;
  std::vector<Instance *> instances;
  std::vector<Resource> resources;
#ifdef CONFIG_DAWN_IO_NOTIFY
  std::vector<NotifyContext *> notifyContexts;
#endif

  /** @brief Parse one descriptor item into resource mappings. */

  int configureDesc(const SObjectCfg::SObjectCfgItem *item);

  /** @brief Add one resource mapping to this LwM2M object. */

  int allocObject(const SProtoWakaamaIOBind &bind);

  /** @brief Find a mapped resource by LwM2M instance and resource ID. */

  Resource *findResource(uint16_t instanceId, uint16_t resourceId);

  /** @brief Find or create a Wakaama instance node for mapped resources. */

  Instance *findOrCreateInstance(uint16_t instanceId);

  /** @brief Read a mapped Dawn IO value into a Wakaama data item. */

  int readResource(Resource &res, lwm2m_data_t &data);

  /** @brief Write a Wakaama data item into a mapped Dawn IO. */

  int writeResource(Resource &res, const lwm2m_data_t &data);

  /** @brief Execute an action on a mapped Dawn IO resource. */

  int executeResource(Resource &res, const uint8_t *buffer, int length);
#ifdef CONFIG_DAWN_IO_NOTIFY
  /** @brief Subscribe mapped notify-capable IOs for LwM2M observations. */

  void setupNotifications();

  /** @brief Remove IO notification subscriptions for this binding. */

  void destroyNotifications();

  /** @brief Dawn IO notification callback that queues LwM2M observe updates. */

  static int notifierCb(void *priv, io_ddata_t *data);
#endif

  /** @brief Wakaama read callback for object resources. */

  static uint8_t readCb(lwm2m_context_t *ctx,
                        uint16_t instanceId,
                        int *numData,
                        lwm2m_data_t **dataArray,
                        lwm2m_object_t *object);

  /** @brief Wakaama discover callback for object resources. */

  static uint8_t discoverCb(lwm2m_context_t *ctx,
                            uint16_t instanceId,
                            int *numData,
                            lwm2m_data_t **dataArray,
                            lwm2m_object_t *object);

  /** @brief Wakaama write callback for writable object resources. */

  static uint8_t writeCb(lwm2m_context_t *ctx,
                         uint16_t instanceId,
                         int numData,
                         lwm2m_data_t *dataArray,
                         lwm2m_object_t *object,
                         lwm2m_write_type_t writeType);

  /** @brief Wakaama execute callback for executable object resources. */

  static uint8_t executeCb(lwm2m_context_t *ctx,
                           uint16_t instanceId,
                           uint16_t resourceId,
                           uint8_t *buffer,
                           int length,
                           lwm2m_object_t *object);
};

} // namespace wakaama_internal
} // namespace dawn
