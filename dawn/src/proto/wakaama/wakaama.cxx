// dawn/src/proto/wakaama/wakaama.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/wakaama/wakaama.hxx"
#include "client.hxx"
#include "internal.hxx"
#include "object_binding.hxx"
#include "transport.hxx"

#include <cerrno>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <new>

using namespace dawn;
using namespace dawn::wakaama_internal;

namespace
{
void assignDescriptorString(std::string &dst,
                            const SObjectCfg::SObjectCfgItem *item,
                            size_t wordOffset,
                            size_t wordCount);
int hexNibble(char ch);
bool parseHexKey(const char *hex, size_t len, std::vector<uint8_t> &key);
} // namespace

CProtoWakaama::CProtoWakaama(CDescObject &desc)
  : CProtoCommon(desc)
  , runtime(nullptr)
  , transport(nullptr)
  , endpoint(CONFIG_DAWN_PROTO_WAKAAMA_ENDPOINT)
  , serverHost(CONFIG_DAWN_PROTO_WAKAAMA_SERVER_HOST)
  , manufacturer(CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_MANUFACTURER)
  , modelNumber(CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_MODEL_NUMBER)
  , serialNumber(CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_SERIAL_NUMBER)
  , firmwareVersion(CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_FIRMWARE_VERSION)
  , serverPort(CONFIG_DAWN_PROTO_WAKAAMA_SERVER_PORT)
  , localPort(CONFIG_DAWN_PROTO_WAKAAMA_LOCAL_PORT)
  , lifetime(CONFIG_DAWN_PROTO_WAKAAMA_LIFETIME)
  , shortServerId(CONFIG_DAWN_PROTO_WAKAAMA_SHORT_SERVER_ID)
#ifdef CONFIG_DAWN_IO_NOTIFY
  , changedResourcesCapacity(0)
  , acceptChangedResources(false)
#endif
{
}

CProtoWakaama::~CProtoWakaama()
{
  deinit();

  for (ObjectBinding *obj : objects)
    {
      delete obj;
    }

  objects.clear();
}

int CProtoWakaama::configureDesc(const CDescObject &desc)
{
  const SObjectCfg::SObjectCfgItem *item;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_WAKAAMA)
        {
          DAWNERR("Unsupported Wakaama config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_WAKAAMA_CFG_ENDPOINT:
            {
              assignDescriptorString(endpoint, item, 0, item->cfgid.s.size);
              break;
            }

          case PROTO_WAKAAMA_CFG_SERVER_HOST:
            {
              assignDescriptorString(serverHost, item, 0, item->cfgid.s.size);
              break;
            }

          case PROTO_WAKAAMA_CFG_SERVER_PORT:
            {
              serverPort = static_cast<uint16_t>(item->data[0]);
              break;
            }

          case PROTO_WAKAAMA_CFG_LOCAL_PORT:
            {
              localPort = static_cast<uint16_t>(item->data[0]);
              break;
            }

          case PROTO_WAKAAMA_CFG_LIFETIME:
            {
              lifetime = item->data[0];
              break;
            }

          case PROTO_WAKAAMA_CFG_IOBIND:
            {
              ObjectBinding *obj = new (std::nothrow) ObjectBinding(item, this);
              int ret;

              if (obj == nullptr)
                {
                  return -ENOMEM;
                }

              ret = obj->configureStatus();
              if (ret < 0)
                {
                  delete obj;
                  return ret;
                }

#ifdef CONFIG_DAWN_IO_NOTIFY
              changedResourcesCapacity +=
                item->cfgid.s.size / (sizeof(SProtoWakaamaIOBind) / sizeof(uint32_t));
#endif
              objects.push_back(obj);
              break;
            }

          case PROTO_WAKAAMA_CFG_DEVICE_MANUFACTURER:
            {
              assignDescriptorString(manufacturer, item, 0, item->cfgid.s.size);
              break;
            }

          case PROTO_WAKAAMA_CFG_DEVICE_MODEL_NUMBER:
            {
              assignDescriptorString(modelNumber, item, 0, item->cfgid.s.size);
              break;
            }

          case PROTO_WAKAAMA_CFG_DEVICE_SERIAL_NUMBER:
            {
              assignDescriptorString(serialNumber, item, 0, item->cfgid.s.size);
              break;
            }

          case PROTO_WAKAAMA_CFG_DEVICE_FIRMWARE_VERSION:
            {
              assignDescriptorString(firmwareVersion, item, 0, item->cfgid.s.size);
              break;
            }

          case PROTO_WAKAAMA_CFG_DEVICE_BATTERY_VOLTAGE:
            {
              setDeviceBatteryBind(WAKAAMA_DEVICE_RESOURCE_POWER_SOURCE_VOLTAGE, item->data[0]);
              break;
            }

          case PROTO_WAKAAMA_CFG_DEVICE_BATTERY_LEVEL:
            {
              setDeviceBatteryBind(WAKAAMA_DEVICE_RESOURCE_BATTERY_LEVEL, item->data[0]);
              break;
            }

          case PROTO_WAKAAMA_CFG_DEVICE_BATTERY_STATUS:
            {
              setDeviceBatteryBind(WAKAAMA_DEVICE_RESOURCE_BATTERY_STATUS, item->data[0]);
              break;
            }

          case PROTO_WAKAAMA_CFG_SERVER:
            {
              int ret = configureServer(item);
              if (ret < 0)
                {
                  return ret;
                }

              break;
            }

          default:
            {
              DAWNERR("Unsupported Wakaama config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CProtoWakaama::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Wakaama configure failed (error %d)\n", ret);
      return ret;
    }

  if (servers.empty())
    {
      addDefaultServer();
    }

  for (const ServerConfig &server : servers)
    {
      if (server.scheme == WAKAAMA_SERVER_SCHEME_COAPS ||
          server.securityMode == LWM2M_SECURITY_MODE_PRE_SHARED_KEY)
        {
#ifndef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
          DAWNERR("Wakaama coaps/PSK server requires CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK\n");
          return -ENOTSUP;
#endif
        }

      if (server.bootstrap)
        {
#ifndef CONFIG_WAKAAMA_BOOTSTRAP
          DAWNERR("Wakaama bootstrap server requires CONFIG_WAKAAMA_BOOTSTRAP\n");
          return -ENOTSUP;
#endif
        }
    }

  return OK;
}

int CProtoWakaama::configureServer(const SObjectCfg::SObjectCfgItem *item)
{
  ServerConfig server;
  size_t pos;
  uint32_t flags;

  if (item == nullptr || item->cfgid.s.size < 3)
    {
      return -EINVAL;
    }

  server.host = serverHost;
  server.pskIdentity.clear();
  server.pskKey.clear();
  server.port = static_cast<uint16_t>(item->data[1] & 0xffff);
  server.lifetime = item->data[2];
  server.shortServerId = static_cast<uint16_t>((item->data[1] >> 16) & 0xffff);
  server.securityInstanceId = static_cast<uint16_t>((item->data[0] >> 16) & 0xffff);
  server.serverInstanceId = static_cast<uint16_t>(item->data[0] & 0xffff);
  server.holdoff = 10;
  server.bootstrapTimeout = 0;
  server.scheme = WAKAAMA_SERVER_SCHEME_COAP;
  server.securityMode = LWM2M_SECURITY_MODE_NONE;
  server.bootstrap = false;

  if (item->cfgid.s.size > 3 && item->data[3] == WAKAAMA_SERVER_EXT_MAGIC)
    {
      size_t identityWords;
      size_t keyWords;
      size_t hostWords;
      const char *identity;
      const char *key;

      if (item->cfgid.s.size < 9)
        {
          return -EINVAL;
        }

      flags = item->data[4];
      server.scheme = (flags & WAKAAMA_SERVER_FLAG_COAPS) != 0 ? WAKAAMA_SERVER_SCHEME_COAPS
                                                               : WAKAAMA_SERVER_SCHEME_COAP;
      server.securityMode = static_cast<uint8_t>((flags >> WAKAAMA_SERVER_FLAG_SECURITY_SHIFT) &
                                                 WAKAAMA_SERVER_FLAG_SECURITY_MASK);
      server.bootstrap = (flags & WAKAAMA_SERVER_FLAG_BOOTSTRAP) != 0;
      server.holdoff = item->data[5];
      server.bootstrapTimeout = item->data[6];
      identityWords = item->data[7];
      keyWords = item->data[8];
      hostWords = item->cfgid.s.size > 9 ? item->data[9] : 0;
      pos = 10;

      if (item->cfgid.s.size < pos + identityWords + keyWords + hostWords)
        {
          return -EINVAL;
        }

      identity = reinterpret_cast<const char *>(&item->data[pos]);
      server.pskIdentity.assign(identity, identityWords * sizeof(uint32_t));
      server.pskIdentity = server.pskIdentity.c_str();
      pos += identityWords;

      key = reinterpret_cast<const char *>(&item->data[pos]);
      if (!parseHexKey(key, keyWords * sizeof(uint32_t), server.pskKey))
        {
          return -EINVAL;
        }
      pos += keyWords;

      if (hostWords > 0)
        {
          assignDescriptorString(server.host, item, pos, hostWords);
        }
    }
  else if (item->cfgid.s.size > 3)
    {
      assignDescriptorString(server.host, item, 3, item->cfgid.s.size - 3);
    }

  if (server.securityMode == LWM2M_SECURITY_MODE_PRE_SHARED_KEY &&
      (server.pskIdentity.empty() || server.pskKey.empty()))
    {
      return -EINVAL;
    }

  servers.push_back(server);
  return OK;
}

void CProtoWakaama::addDefaultServer()
{
  ServerConfig server;

  server.host = serverHost;
  server.pskIdentity.clear();
  server.pskKey.clear();
  server.port = serverPort;
  server.lifetime = lifetime;
  server.shortServerId = shortServerId;
  server.securityInstanceId = 0;
  server.serverInstanceId = 0;
  server.holdoff = 10;
  server.bootstrapTimeout = 0;
  server.scheme = WAKAAMA_SERVER_SCHEME_COAP;
  server.securityMode = LWM2M_SECURITY_MODE_NONE;
  server.bootstrap = false;
  servers.push_back(server);
}

size_t CProtoWakaama::serverPoolCapacity() const
{
  size_t capacity = servers.size();

  for (const ServerConfig &server : servers)
    {
      if (server.bootstrap)
        {
          capacity++;
        }
    }

  return capacity;
}

std::string CProtoWakaama::serverUri(const ServerConfig &server) const
{
  return std::string(server.scheme == WAKAAMA_SERVER_SCHEME_COAPS ? "coaps://" : "coap://") +
         server.host + ":" + std::to_string(server.port);
}

const CProtoWakaama::ServerConfig *CProtoWakaama::findServer(uint16_t securityInstanceId) const
{
  for (const ServerConfig &server : servers)
    {
      if (server.securityInstanceId == securityInstanceId)
        {
          return &server;
        }
    }

  return nullptr;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
void CProtoWakaama::queueResourceChanged(uint16_t objectId,
                                         uint16_t instanceId,
                                         uint16_t resourceId)
{
  std::lock_guard<std::mutex> lock(changedResourcesMutex);

  if (!acceptChangedResources.load())
    {
      return;
    }

  for (const ChangedResource &res : changedResources)
    {
      if (res.objectId == objectId && res.instanceId == instanceId && res.resourceId == resourceId)
        {
          return;
        }
    }

  if (changedResources.size() >= changedResources.capacity())
    {
      DAWNERR("Wakaama changed resource queue full\n");
      return;
    }

  changedResources.push_back({objectId, instanceId, resourceId});
}

void CProtoWakaama::processChangedResources()
{
  if (runtime == nullptr || runtime->context() == nullptr || !acceptChangedResources.load())
    {
      return;
    }

  std::lock_guard<std::mutex> lock(changedResourcesMutex);

  for (const ChangedResource &res : changedResources)
    {
      lwm2m_uri_t uri;

      LWM2M_URI_RESET(&uri);
      uri.objectId = res.objectId;
      uri.instanceId = res.instanceId;
      uri.resourceId = res.resourceId;
      lwm2m_resource_value_changed(runtime->context(), &uri);
    }

  changedResources.clear();
}
#endif

CProtoWakaama::SDeviceIoBind *CProtoWakaama::deviceBatteryBind(uint16_t resourceId)
{
  switch (resourceId)
    {
      case WAKAAMA_DEVICE_RESOURCE_POWER_SOURCE_VOLTAGE:
        return &devBattVoltage;
      case WAKAAMA_DEVICE_RESOURCE_BATTERY_LEVEL:
        return &devBattLevel;
      case WAKAAMA_DEVICE_RESOURCE_BATTERY_STATUS:
        return &devBattStatus;
      default:
        return nullptr;
    }
}

void CProtoWakaama::setDeviceBatteryBind(uint16_t resourceId, SObjectId::ObjectId objid)
{
  SDeviceIoBind *bind = deviceBatteryBind(resourceId);
  if (bind == nullptr)
    {
      return;
    }

  bind->objid = objid;
  regObject(objid);
}

int CProtoWakaama::buildObjects()
{
  runtime = new (std::nothrow) ClientRuntime(*this);
  if (runtime == nullptr)
    {
      return -ENOMEM;
    }

  return runtime->build(objects);
}

void CProtoWakaama::destroyObjects()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  acceptChangedResources.store(false);
#endif

  if (runtime != nullptr)
    {
      runtime->destroy(objects);
    }

  destroyDtls();

#ifdef CONFIG_DAWN_IO_NOTIFY
  {
    std::lock_guard<std::mutex> lock(changedResourcesMutex);
    changedResources.clear();
  }
#endif

  delete runtime;
  runtime = nullptr;
}

int CProtoWakaama::init()
{
  int ret;

  ret = buildObjects();
  if (ret < 0)
    {
      deinit();
      return ret;
    }

  ret = initConnectionPool();
  if (ret < 0)
    {
      deinit();
      return ret;
    }

  ret = openSocket();
  if (ret < 0)
    {
      deinit();
      return ret;
    }

  ret = runtime->openContext(transport);
  if (ret < 0)
    {
      deinit();
      return ret;
    }

  ret = initDtls();
  if (ret < 0)
    {
      deinit();
      return ret;
    }

  ret = runtime->configure(endpoint.c_str());
  if (ret != 0)
    {
      DAWNERR("lwm2m_configure failed: %d\n", ret);
      deinit();
      return -EIO;
    }

#ifdef CONFIG_DAWN_IO_NOTIFY
  changedResources.reserve(changedResourcesCapacity);
#endif

#ifdef CONFIG_DAWN_IO_NOTIFY
  acceptChangedResources.store(true);
#endif

  return OK;
}

int CProtoWakaama::deinit()
{
  stop();
  destroyObjects();
  destroyConnectionPool();

  return OK;
}

int CProtoWakaama::doStart()
{
  int ret;

  /* The Wakaama thread keeps a full RX_BUFFER_SIZE packet buffer on its
   * stack and runs the LwM2M/CoAP step, so it needs more than the default
   * pthread stack.
   */

  setThreadStackSize(CONFIG_DAWN_PROTO_WAKAAMA_STACKSIZE);

  ret = startWorkerThread([this]() { thread(); });
  if (ret < 0)
    {
      DAWNERR("failed to start Wakaama thread %d\n", ret);
      return ret;
    }

  return OK;
}

int CProtoWakaama::doStop()
{
  stopWorkerThread();

  return OK;
}

bool CProtoWakaama::hasThread() const
{
  return workerThreadRunning();
}

namespace
{
void assignDescriptorString(std::string &dst,
                            const SObjectCfg::SObjectCfgItem *item,
                            size_t wordOffset,
                            size_t wordCount)
{
  dst.assign(reinterpret_cast<const char *>(&item->data[wordOffset]), wordCount * sizeof(uint32_t));
  size_t nul = dst.find('\0');
  if (nul != std::string::npos)
    {
      dst.resize(nul);
    }
}

int hexNibble(char ch)
{
  if (ch >= '0' && ch <= '9')
    {
      return ch - '0';
    }

  ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
  if (ch >= 'a' && ch <= 'f')
    {
      return ch - 'a' + 10;
    }

  return -1;
}

bool parseHexKey(const char *hex, size_t len, std::vector<uint8_t> &key)
{
  while (len > 0 && hex[len - 1] == '\0')
    {
      len--;
    }

  if (len == 0)
    {
      key.clear();
      return true;
    }

  if ((len % 2) != 0)
    {
      return false;
    }

  key.clear();
  key.reserve(len / 2);
  for (size_t i = 0; i < len; i += 2)
    {
      int hi = hexNibble(hex[i]);
      int lo = hexNibble(hex[i + 1]);

      if (hi < 0 || lo < 0)
        {
          key.clear();
          return false;
        }

      key.push_back(static_cast<uint8_t>((hi << 4) | lo));
    }

  return true;
}

} // namespace
