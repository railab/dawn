// dawn/src/proto/wakaama/client.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "client.hxx"

#include <cerrno>
#include <cstring>
#include <new>

using namespace dawn;
using namespace dawn::wakaama_internal;

ClientRuntime::ClientRuntime(CProtoWakaama &owner_)
  : owner(owner_)
  , ctx(nullptr)
  , securityObj(nullptr)
  , serverObj(nullptr)
  , deviceObj(nullptr)
  , instancePools(nullptr)
{
}

ClientRuntime::~ClientRuntime()
{
  closeContext();
  destroy({});
}

int ClientRuntime::build(const std::vector<ObjectBinding *> &objects)
{
  int ret;

  securityObj = new (std::nothrow) lwm2m_object_t();
  serverObj = new (std::nothrow) lwm2m_object_t();
  deviceObj = new (std::nothrow) lwm2m_object_t();
  if (securityObj == nullptr || serverObj == nullptr || deviceObj == nullptr)
    {
      return -ENOMEM;
    }

  ret = buildSecurityAndServerObjects();
  if (ret < 0)
    {
      return ret;
    }

  ret = buildDeviceObject();
  if (ret < 0)
    {
      return ret;
    }

  lwm2mObjects.push_back(securityObj);
  lwm2mObjects.push_back(serverObj);
  lwm2mObjects.push_back(deviceObj);

  for (ObjectBinding *obj : objects)
    {
      ret = obj->init();
      if (ret < 0)
        {
          return ret;
        }

      lwm2mObjects.push_back(obj->object());
    }

  return OK;
}

int ClientRuntime::buildSecurityAndServerObjects()
{
  InstancePools *pools;
  const size_t serverCapacity = owner.serverPoolCapacity();

  pools = new (std::nothrow) InstancePools();
  if (pools == nullptr)
    {
      return -ENOMEM;
    }

  pools->securityCapacity = serverCapacity;
  pools->serverCapacity = serverCapacity;
  pools->security = new (std::nothrow) security_instance_s[serverCapacity]();
  pools->server = new (std::nothrow) server_instance_s[serverCapacity]();
  if (pools->security == nullptr || pools->server == nullptr)
    {
      delete[] pools->security;
      delete[] pools->server;
      delete pools;
      return -ENOMEM;
    }

  instancePools = pools;
  for (const CProtoWakaama::ServerConfig &server : owner.servers)
    {
      security_instance_s *secInst;
      server_instance_s *srvInst = nullptr;
      std::string uri = owner.serverUri(server);

      secInst = allocateSecurityInstance(pools);
      if (!server.bootstrap)
        {
          srvInst = allocateServerInstance(pools);
        }

      if (secInst == nullptr || (!server.bootstrap && srvInst == nullptr))
        {
          return -ENOMEM;
        }

      secInst->id = server.securityInstanceId;
      secInst->shortServerId = server.shortServerId;
      secInst->holdoff = server.holdoff;
      secInst->bootstrapTimeout = server.bootstrapTimeout;
      secInst->securityMode = server.securityMode;
      secInst->bootstrap = server.bootstrap;
      if (!assignSecurityString(
            *secInst, reinterpret_cast<const uint8_t *>(uri.c_str()), uri.size()) ||
          !assignSecurityBuffer(reinterpret_cast<const uint8_t *>(server.pskIdentity.data()),
                                server.pskIdentity.size(),
                                secInst->publicIdentityBuffer,
                                sizeof(secInst->publicIdentityBuffer),
                                &secInst->publicIdentity,
                                &secInst->publicIdentityLen) ||
          !assignSecurityBuffer(server.pskKey.data(),
                                server.pskKey.size(),
                                secInst->secretKeyBuffer,
                                sizeof(secInst->secretKeyBuffer),
                                &secInst->secretKey,
                                &secInst->secretKeyLen))
        {
          return -ENOMEM;
        }

      securityObj->instanceList = LWM2M_LIST_ADD(securityObj->instanceList, secInst);

      if (!server.bootstrap)
        {
          srvInst->id = server.serverInstanceId;
          srvInst->shortServerId = server.shortServerId;
          srvInst->lifetime = server.lifetime;
          srvInst->storing = false;

          serverObj->instanceList = LWM2M_LIST_ADD(serverObj->instanceList, srvInst);
        }
    }

  securityObj->objID = LWM2M_SECURITY_OBJECT_ID;
  securityObj->userData = pools;
  securityObj->readFunc = securityRead;
  securityObj->discoverFunc = securityDiscover;
#ifdef CONFIG_WAKAAMA_BOOTSTRAP
  securityObj->writeFunc = securityWrite;
  securityObj->createFunc = securityCreate;
  securityObj->deleteFunc = securityDelete;
#endif

  serverObj->objID = LWM2M_SERVER_OBJECT_ID;
  serverObj->userData = pools;
  serverObj->readFunc = serverRead;
  serverObj->discoverFunc = serverDiscover;
  serverObj->executeFunc = serverExecute;
  serverObj->writeFunc = serverWrite;
#ifdef CONFIG_WAKAAMA_BOOTSTRAP
  serverObj->createFunc = serverCreate;
  serverObj->deleteFunc = serverDelete;
#endif

  return OK;
}

int ClientRuntime::buildDeviceObject()
{
  device_instance_s *devInst = new (std::nothrow) device_instance_s();
  if (devInst == nullptr)
    {
      return -ENOMEM;
    }

  devInst->id = 0;
  deviceObj->objID = CProtoWakaama::WAKAAMA_OBJECT_DEVICE;
  deviceObj->instanceList = LWM2M_LIST_ADD(deviceObj->instanceList, devInst);
  deviceObj->userData = &owner;
  deviceObj->readFunc = deviceRead;
  deviceObj->discoverFunc = deviceDiscover;

  return OK;
}

void ClientRuntime::destroy(const std::vector<ObjectBinding *> &objects)
{
  closeContext();

  for (ObjectBinding *obj : objects)
    {
      obj->deinit();
    }

  lwm2mObjects.clear();

  if (securityObj != nullptr)
    {
      securityObj->instanceList = nullptr;
      delete securityObj;
      securityObj = nullptr;
    }

  if (serverObj != nullptr)
    {
      serverObj->instanceList = nullptr;
      delete serverObj;
      serverObj = nullptr;
    }

  if (deviceObj != nullptr)
    {
      while (deviceObj->instanceList != nullptr)
        {
          device_instance_s *inst = reinterpret_cast<device_instance_s *>(deviceObj->instanceList);

          deviceObj->instanceList = deviceObj->instanceList->next;
          delete inst;
        }

      delete deviceObj;
      deviceObj = nullptr;
    }

  if (instancePools != nullptr)
    {
      delete[] instancePools->security;
      delete[] instancePools->server;
      delete instancePools;
      instancePools = nullptr;
    }
}

int ClientRuntime::openContext(void *userdata)
{
  ctx = lwm2m_init(userdata);

  return ctx == nullptr ? -ENOMEM : OK;
}

int ClientRuntime::configure(const char *endpoint)
{
  if (ctx == nullptr)
    {
      return -ENODEV;
    }

  return lwm2m_configure(ctx,
                         endpoint,
                         nullptr,
                         nullptr,
                         static_cast<uint16_t>(lwm2mObjects.size()),
                         lwm2mObjects.data());
}

int ClientRuntime::step(time_t *timeout)
{
  return ctx == nullptr ? -ENODEV : lwm2m_step(ctx, timeout);
}

bool ClientRuntime::ready() const
{
  return ctx != nullptr && ctx->state == STATE_READY;
}

void ClientRuntime::handlePacket(uint8_t *buffer, size_t length, void *session)
{
  if (ctx != nullptr && session != nullptr)
    {
      lwm2m_handle_packet(ctx, buffer, length, session);
    }
}

security_instance_s *ClientRuntime::findSecurityInstance(uint16_t securityInstanceId) const
{
  if (securityObj == nullptr)
    {
      return nullptr;
    }

  return reinterpret_cast<security_instance_s *>(
    lwm2m_list_find(securityObj->instanceList, securityInstanceId));
}

void ClientRuntime::closeContext()
{
  if (ctx != nullptr)
    {
      lwm2m_close(ctx);
      ctx = nullptr;
    }
}
