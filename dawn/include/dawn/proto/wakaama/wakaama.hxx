// dawn/include/dawn/proto/wakaama/wakaama.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <mutex>
#include <string>
#include <vector>

#include "dawn/common/thread.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"

namespace dawn
{
namespace wakaama_internal
{
class ClientRuntime;
class ObjectBinding;
class Transport;
}

/**
 * @brief Descriptor binding for one LwM2M resource.
 */

struct SProtoWakaamaIOBind
{
  uint16_t objectId;
  uint16_t instanceId;
  uint16_t resourceId;
  uint16_t access;
  SObjectId::ObjectId objid;
};

/**
 * @brief Wakaama LwM2M client protocol implementation.
 *
 * CProtoWakaama owns the Wakaama client context and network transport. LwM2M
 * object/resource mappings are represented by per-object runtime classes.
 */

class CProtoWakaama
  : public CProtoCommon
  , protected CThreadedObject
{
public:
  enum
  {
    PROTO_WAKAAMA_ACCESS_READ = (1 << 0),
    PROTO_WAKAAMA_ACCESS_WRITE = (1 << 1),
    PROTO_WAKAAMA_ACCESS_EXECUTE = (1 << 2),
    PROTO_WAKAAMA_ACCESS_RW = 3,
    PROTO_WAKAAMA_ACCESS_RWE = 7,
  };

  static constexpr size_t RX_BUFFER_SIZE = 2048;
  static constexpr size_t IO_CHUNK_CAP = RX_BUFFER_SIZE;

  enum
  {
    WAKAAMA_OBJECT_DEVICE = 3,
    WAKAAMA_OBJECT_CONNECTIVITY_MONITORING = 4,
    WAKAAMA_OBJECT_FIRMWARE_UPDATE = 5,
    WAKAAMA_OBJECT_SOFTWARE_MANAGEMENT = 9,
    WAKAAMA_OBJECT_CELLULAR_CONNECTIVITY = 10,
    WAKAAMA_OBJECT_BINARY_APP_DATA_CONTAINER = 19,
    WAKAAMA_OBJECT_DIGITAL_INPUT = 3200,
    WAKAAMA_OBJECT_DIGITAL_OUTPUT = 3201,
    WAKAAMA_OBJECT_ANALOG_INPUT = 3202,
    WAKAAMA_OBJECT_ANALOG_OUTPUT = 3203,
    WAKAAMA_OBJECT_GENERIC_SENSOR = 3300,
    WAKAAMA_OBJECT_ILLUMINANCE = 3301,
    WAKAAMA_OBJECT_LIGHT = 3301,
    WAKAAMA_OBJECT_TEMPERATURE = 3303,
    WAKAAMA_OBJECT_HUMIDITY = 3304,
    WAKAAMA_OBJECT_ACTUATION = 3306,
    WAKAAMA_OBJECT_LIGHT_CONTROL = 3311,
    WAKAAMA_OBJECT_ACCELEROMETER = 3313,
    WAKAAMA_OBJECT_MAGNETOMETER = 3314,
    WAKAAMA_OBJECT_BAROMETER = 3315,
    WAKAAMA_OBJECT_VOLTAGE = 3316,
    WAKAAMA_OBJECT_CURRENT = 3317,
    WAKAAMA_OBJECT_PRESSURE = 3323,
    WAKAAMA_OBJECT_GYROMETER = 3334,
  };

  enum
  {
    WAKAAMA_DEVICE_RESOURCE_MANUFACTURER = 0,
    WAKAAMA_DEVICE_RESOURCE_MODEL_NUMBER = 1,
    WAKAAMA_DEVICE_RESOURCE_SERIAL_NUMBER = 2,
    WAKAAMA_DEVICE_RESOURCE_FIRMWARE_VERSION = 3,
    WAKAAMA_DEVICE_RESOURCE_ERROR_CODE = 11,
    WAKAAMA_DEVICE_RESOURCE_CURRENT_TIME = 13,
    WAKAAMA_DEVICE_RESOURCE_BINDING_MODES = 16,
  };

  enum
  {
    WAKAAMA_RESOURCE_BINARY_APP_DATA = 0,
    WAKAAMA_RESOURCE_BINARY_APP_DATA_PRIORITY = 1,
    WAKAAMA_RESOURCE_BINARY_APP_DATA_CREATION_TIME = 2,
    WAKAAMA_RESOURCE_BINARY_APP_DATA_DESCRIPTION = 3,
    WAKAAMA_RESOURCE_BINARY_APP_DATA_FORMAT = 4,
    WAKAAMA_RESOURCE_BINARY_APP_ID = 5,

    WAKAAMA_RESOURCE_FIRMWARE_PACKAGE = 0,
    WAKAAMA_RESOURCE_FIRMWARE_PACKAGE_URI = 1,
    WAKAAMA_RESOURCE_FIRMWARE_UPDATE = 2,
    WAKAAMA_RESOURCE_FIRMWARE_STATE = 3,
    WAKAAMA_RESOURCE_FIRMWARE_UPDATE_RESULT = 5,
    WAKAAMA_RESOURCE_FIRMWARE_PACKAGE_NAME = 6,
    WAKAAMA_RESOURCE_FIRMWARE_PACKAGE_VERSION = 7,
    WAKAAMA_RESOURCE_FIRMWARE_PROTOCOL_SUPPORT = 8,
    WAKAAMA_RESOURCE_FIRMWARE_DELIVERY_METHOD = 9,
    WAKAAMA_RESOURCE_FIRMWARE_CANCEL = 10,

    WAKAAMA_RESOURCE_SOFTWARE_PACKAGE_NAME = 0,
    WAKAAMA_RESOURCE_SOFTWARE_PACKAGE_VERSION = 1,
    WAKAAMA_RESOURCE_SOFTWARE_PACKAGE = 2,
    WAKAAMA_RESOURCE_SOFTWARE_PACKAGE_URI = 3,
    WAKAAMA_RESOURCE_SOFTWARE_INSTALL = 4,
    WAKAAMA_RESOURCE_SOFTWARE_UNINSTALL = 6,
    WAKAAMA_RESOURCE_SOFTWARE_UPDATE_STATE = 7,
    WAKAAMA_RESOURCE_SOFTWARE_UPDATE_RESULT = 9,
    WAKAAMA_RESOURCE_SOFTWARE_ACTIVATE = 10,
    WAKAAMA_RESOURCE_SOFTWARE_DEACTIVATE = 11,
    WAKAAMA_RESOURCE_SOFTWARE_ACTIVATION_STATE = 12,

    WAKAAMA_RESOURCE_DIGITAL_INPUT_STATE = 5500,
    WAKAAMA_RESOURCE_DIGITAL_INPUT_COUNTER = 5501,
    WAKAAMA_RESOURCE_DIGITAL_INPUT_POLARITY = 5502,
    WAKAAMA_RESOURCE_DIGITAL_INPUT_DEBOUNCE = 5503,
    WAKAAMA_RESOURCE_DIGITAL_INPUT_EDGE_SELECTION = 5504,
    WAKAAMA_RESOURCE_DIGITAL_INPUT_COUNTER_RESET = 5505,
    WAKAAMA_RESOURCE_DIGITAL_OUTPUT_STATE = 5550,
    WAKAAMA_RESOURCE_DIGITAL_OUTPUT_POLARITY = 5551,
    WAKAAMA_RESOURCE_ANALOG_INPUT_CURRENT_VALUE = 5600,
    WAKAAMA_RESOURCE_ANALOG_OUTPUT_CURRENT_VALUE = 5650,
    WAKAAMA_RESOURCE_SENSOR_VALUE = 5700,
    WAKAAMA_RESOURCE_UNITS = 5701,
    WAKAAMA_RESOURCE_SENSOR_UNITS = 5701,
    WAKAAMA_RESOURCE_COLOUR = 5706,
    WAKAAMA_RESOURCE_COLOR = 5706,
    WAKAAMA_RESOURCE_APPLICATION_TYPE = 5750,
    WAKAAMA_RESOURCE_SENSOR_TYPE = 5751,
    WAKAAMA_RESOURCE_MIN_MEASURED_VALUE = 5601,
    WAKAAMA_RESOURCE_MAX_MEASURED_VALUE = 5602,
    WAKAAMA_RESOURCE_MIN_RANGE_VALUE = 5603,
    WAKAAMA_RESOURCE_MAX_RANGE_VALUE = 5604,
    WAKAAMA_RESOURCE_RESET_MIN_MAX_MEASURED_VALUES = 5605,
    WAKAAMA_RESOURCE_X_VALUE = 5702,
    WAKAAMA_RESOURCE_Y_VALUE = 5703,
    WAKAAMA_RESOURCE_Z_VALUE = 5704,
    WAKAAMA_RESOURCE_TIMESTAMP = 5518,
    WAKAAMA_RESOURCE_ON_OFF = 5850,
    WAKAAMA_RESOURCE_DIMMER = 5851,
    WAKAAMA_RESOURCE_ON_TIME = 5852,
  };

  enum
  {
    PROTO_WAKAAMA_CFG_FIRST = 0,
    PROTO_WAKAAMA_CFG_ENDPOINT = 1,
    PROTO_WAKAAMA_CFG_SERVER_HOST = 2,
    PROTO_WAKAAMA_CFG_SERVER_PORT = 3,
    PROTO_WAKAAMA_CFG_LOCAL_PORT = 4,
    PROTO_WAKAAMA_CFG_LIFETIME = 5,
    PROTO_WAKAAMA_CFG_IOBIND = 6,
    PROTO_WAKAAMA_CFG_DEVICE_MANUFACTURER = 7,
    PROTO_WAKAAMA_CFG_DEVICE_MODEL_NUMBER = 8,
    PROTO_WAKAAMA_CFG_DEVICE_SERIAL_NUMBER = 9,
    PROTO_WAKAAMA_CFG_DEVICE_FIRMWARE_VERSION = 10,
    PROTO_WAKAAMA_CFG_SERVER = 11,
    PROTO_WAKAAMA_CFG_LAST = 31
  };

  enum
  {
    WAKAAMA_SERVER_SCHEME_COAP = 0,
    WAKAAMA_SERVER_SCHEME_COAPS = 1,
  };

  explicit CProtoWakaama(CDescObject &desc);
  ~CProtoWakaama() override;

#ifdef CONFIG_DAWN_OBJECT_HAS_NAME
  const char *getClassNameStr() const override
  {
    return "wakaama";
  }
#endif

  int configure() override;
  int init() override;
  int deinit() override;
  int doStart() override;
  int doStop() override;
  bool hasThread() const override;

  const char *deviceManufacturer() const
  {
    return manufacturer.c_str();
  }

  const char *deviceModelNumber() const
  {
    return modelNumber.c_str();
  }

  const char *deviceSerialNumber() const
  {
    return serialNumber.c_str();
  }

  const char *deviceFirmwareVersion() const
  {
    return firmwareVersion.c_str();
  }

  void regObject(SObjectId::ObjectId id)
  {
    this->setObjectMapItem(id, nullptr);
  }

  CIOCommon *getObject(SObjectId::ObjectId id)
  {
    return this->getIO(id);
  }

#ifdef CONFIG_DAWN_IO_NOTIFY
  void queueResourceChanged(uint16_t objectId, uint16_t instanceId, uint16_t resourceId);
#endif

  constexpr static SObjectId::ObjectId objectId(uint16_t id)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_WAKAAMA, SObjectId::DTYPE_ANY, 0, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t dtype, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(
      SObjectId::OBJTYPE_PROTO, CProtoCommon::PROTO_CLASS_WAKAAMA, dtype, rw, size, id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdEndpoint(uint16_t size)
  {
    return cfgId(false, SObjectId::DTYPE_CHAR, size, PROTO_WAKAAMA_CFG_ENDPOINT);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdServerHost(uint16_t size)
  {
    return cfgId(false, SObjectId::DTYPE_CHAR, size, PROTO_WAKAAMA_CFG_SERVER_HOST);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdServerPort()
  {
    return cfgId(false, SObjectId::DTYPE_UINT16, 1, PROTO_WAKAAMA_CFG_SERVER_PORT);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdLocalPort()
  {
    return cfgId(false, SObjectId::DTYPE_UINT16, 1, PROTO_WAKAAMA_CFG_LOCAL_PORT);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdLifetime()
  {
    return cfgId(false, SObjectId::DTYPE_UINT32, 1, PROTO_WAKAAMA_CFG_LIFETIME);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind(uint16_t size)
  {
    return cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_WAKAAMA_CFG_IOBIND);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdDeviceManufacturer(uint16_t size)
  {
    return cfgId(false, SObjectId::DTYPE_CHAR, size, PROTO_WAKAAMA_CFG_DEVICE_MANUFACTURER);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdDeviceModelNumber(uint16_t size)
  {
    return cfgId(false, SObjectId::DTYPE_CHAR, size, PROTO_WAKAAMA_CFG_DEVICE_MODEL_NUMBER);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdDeviceSerialNumber(uint16_t size)
  {
    return cfgId(false, SObjectId::DTYPE_CHAR, size, PROTO_WAKAAMA_CFG_DEVICE_SERIAL_NUMBER);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdDeviceFirmwareVersion(uint16_t size)
  {
    return cfgId(false, SObjectId::DTYPE_CHAR, size, PROTO_WAKAAMA_CFG_DEVICE_FIRMWARE_VERSION);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdServer(uint16_t size)
  {
    return cfgId(false, SObjectId::DTYPE_ANY, size, PROTO_WAKAAMA_CFG_SERVER);
  }

private:
  friend class wakaama_internal::ClientRuntime;
  friend class wakaama_internal::ObjectBinding;
  friend class wakaama_internal::Transport;

  struct ServerConfig
  {
    std::string host;
    std::string pskIdentity;
    std::vector<uint8_t> pskKey;
    uint16_t port;
    uint32_t lifetime;
    uint16_t shortServerId;
    uint16_t securityInstanceId;
    uint16_t serverInstanceId;
    uint32_t holdoff;
    uint32_t bootstrapTimeout;
    uint8_t scheme;
    uint8_t securityMode;
    bool bootstrap;
  };

#ifdef CONFIG_DAWN_IO_NOTIFY
  struct ChangedResource
  {
    uint16_t objectId;
    uint16_t instanceId;
    uint16_t resourceId;
  };
#endif

  wakaama_internal::ClientRuntime *runtime;
  wakaama_internal::Transport *transport;
  std::vector<wakaama_internal::ObjectBinding *> objects;
  std::string endpoint;
  std::string serverHost;
  std::string manufacturer;
  std::string modelNumber;
  std::string serialNumber;
  std::string firmwareVersion;
  uint16_t serverPort;
  uint16_t localPort;
  uint32_t lifetime;
  uint16_t shortServerId;
  std::vector<ServerConfig> servers;
#ifdef CONFIG_DAWN_IO_NOTIFY
  std::mutex changedResourcesMutex;
  std::vector<ChangedResource> changedResources;
  size_t changedResourcesCapacity;
  std::atomic_bool acceptChangedResources;
#endif

  int configureDesc(const CDescObject &desc);
  int configureServer(const SObjectCfg::SObjectCfgItem *item);
  void addDefaultServer();
  int buildObjects();
  void destroyObjects();
  int initConnectionPool();
  int initDtls();
  void destroyDtls();
  int openSocket();
  void closeAllConnections();
  void destroyConnectionPool();
  void thread();
  size_t serverPoolCapacity() const;
  std::string serverUri(const ServerConfig &server) const;
  const ServerConfig *findServer(uint16_t securityInstanceId) const;
#ifdef CONFIG_DAWN_IO_NOTIFY
  void processChangedResources();
#endif
};

} // Namespace dawn
