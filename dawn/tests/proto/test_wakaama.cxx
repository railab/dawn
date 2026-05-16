// dawn/tests/proto/test_wakaama.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cerrno>
#include <cstring>

#include "dawn/io/dummy.hxx"
#include "dawn/proto/wakaama/wakaama.hxx"
#include "test_common.hxx"
#include "../../src/proto/wakaama/internal.hxx"

using namespace dawn;
using namespace dawn::wakaama_internal;

static constexpr uint32_t WAKAAMA_TEST_EXT_MAGIC = 0x574b4131;
static constexpr uint32_t WAKAAMA_TEST_FLAG_COAPS = (1 << 0);
static constexpr uint32_t WAKAAMA_TEST_FLAG_BOOTSTRAP = (1 << 16);
static constexpr uint32_t WAKAAMA_TEST_SECURITY_NONE = (3 << 8);
static constexpr uint32_t WAKAAMA_TEST_SECURITY_PSK = (0 << 8);
static constexpr auto WAKAAMA_TEST_DUMMYIO = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 0);

static constexpr uint32_t pack4(char a, char b = '\0', char c = '\0', char d = '\0')
{
  return static_cast<uint8_t>(a) | (static_cast<uint8_t>(b) << 8) |
         (static_cast<uint8_t>(c) << 16) | (static_cast<uint8_t>(d) << 24);
}

static constexpr uint32_t pack16(uint16_t lo, uint16_t hi)
{
  return lo | (static_cast<uint32_t>(hi) << 16);
}

static bool data_has_id(const lwm2m_data_t *data, int count, uint16_t id)
{
  for (int i = 0; i < count; i++)
    {
      if (data[i].id == id)
        {
          return true;
        }
    }

  return false;
}

static lwm2m_data_t *find_data_id(lwm2m_data_t *data, int count, uint16_t id)
{
  for (int i = 0; i < count; i++)
    {
      if (data[i].id == id)
        {
          return &data[i];
        }
    }

  return nullptr;
}

static void add_server_instance(lwm2m_object_t &object,
                                InstancePools &pools,
                                server_instance_s *serverPool,
                                uint16_t instanceId)
{
  std::memset(&object, 0, sizeof(object));
  std::memset(&pools, 0, sizeof(pools));
  std::memset(serverPool, 0, sizeof(*serverPool));

  pools.server = serverPool;
  pools.serverCapacity = 1;
  object.userData = &pools;

  server_instance_s *inst = allocateServerInstance(&pools);

  TEST_ASSERT_NOT_NULL(inst);
  inst->id = instanceId;
  inst->shortServerId = 123;
  inst->lifetime = 60;
  object.instanceList = LWM2M_LIST_ADD(object.instanceList, inst);
}

static void assert_data_int(lwm2m_data_t *data, int count, uint16_t id, int64_t expected)
{
  lwm2m_data_t *entry = find_data_id(data, count, id);
  int64_t value;

  TEST_ASSERT_NOT_NULL(entry);
  TEST_ASSERT_EQUAL(1, lwm2m_data_decode_int(entry, &value));
  TEST_ASSERT_EQUAL(expected, value);
}

static void assert_data_bool(lwm2m_data_t *data, int count, uint16_t id, bool expected)
{
  lwm2m_data_t *entry = find_data_id(data, count, id);
  bool value;

  TEST_ASSERT_NOT_NULL(entry);
  TEST_ASSERT_EQUAL(1, lwm2m_data_decode_bool(entry, &value));
  TEST_ASSERT_EQUAL(expected, value);
}

static void assert_data_string(lwm2m_data_t *data, int count, uint16_t id, const char *expected)
{
  lwm2m_data_t *entry = find_data_id(data, count, id);

  TEST_ASSERT_NOT_NULL(entry);
  TEST_ASSERT_EQUAL(LWM2M_TYPE_STRING, entry->type);
  TEST_ASSERT_EQUAL(strlen(expected), entry->value.asBuffer.length);
  TEST_ASSERT_EQUAL_MEMORY(expected, entry->value.asBuffer.buffer, strlen(expected));
}

static uint32_t g_cfg_wakaama_defaults[] = {
  CProtoWakaama::objectId(0),
  0,
};

static uint32_t g_cfg_wakaama_multiserver[] = {
  CProtoWakaama::objectId(1),
  7,

  CProtoWakaama::cfgIdDeviceManufacturer(1),
  pack4('U', 'n', 'i', 't'),

  CProtoWakaama::cfgIdDeviceModelNumber(2),
  pack4('M', 'o', 'd', 'e'),
  pack4('l'),

  CProtoWakaama::cfgIdDeviceSerialNumber(1),
  pack4('S', 'e', 'r', '1'),

  CProtoWakaama::cfgIdDeviceFirmwareVersion(1),
  pack4('1', '.', '0'),

  CProtoWakaama::cfgIdLocalPort(),
  59030,

  CProtoWakaama::cfgIdServer(3),
  0,
  (123 << 16) | 5683,
  60,

  CProtoWakaama::cfgIdServer(10),
  (1 << 16) | 1,
  5685,
  0,
  WAKAAMA_TEST_EXT_MAGIC,
  WAKAAMA_TEST_FLAG_BOOTSTRAP | WAKAAMA_TEST_SECURITY_NONE,
  1,
  0,
  0,
  0,
  0,
};

static uint32_t g_cfg_wakaama_psk_missing_key[] = {
  CProtoWakaama::objectId(2),
  1,

  CProtoWakaama::cfgIdServer(11),
  0,
  (123 << 16) | 5683,
  60,
  WAKAAMA_TEST_EXT_MAGIC,
  WAKAAMA_TEST_SECURITY_PSK,
  10,
  0,
  1,
  0,
  0,
  pack4('i', 'd'),
};

static uint32_t g_cfg_wakaama_mixed_iobind_object[] = {
  CProtoWakaama::objectId(4),
  1,

  CProtoWakaama::cfgIdIOBind(2 * sizeof(SProtoWakaamaIOBind) / sizeof(uint32_t)),
  pack16(CProtoWakaama::WAKAAMA_OBJECT_TEMPERATURE, 0),
  pack16(CProtoWakaama::WAKAAMA_RESOURCE_SENSOR_VALUE, CProtoWakaama::PROTO_WAKAAMA_ACCESS_READ),
  WAKAAMA_TEST_DUMMYIO,
  pack16(CProtoWakaama::WAKAAMA_OBJECT_HUMIDITY, 0),
  pack16(CProtoWakaama::WAKAAMA_RESOURCE_SENSOR_VALUE, CProtoWakaama::PROTO_WAKAAMA_ACCESS_READ),
  WAKAAMA_TEST_DUMMYIO,
};

#ifndef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
static uint32_t g_cfg_wakaama_coaps_no_dtls[] = {
  CProtoWakaama::objectId(3),
  1,

  CProtoWakaama::cfgIdServer(10),
  0,
  (123 << 16) | 5684,
  60,
  WAKAAMA_TEST_EXT_MAGIC,
  WAKAAMA_TEST_FLAG_COAPS | WAKAAMA_TEST_SECURITY_NONE,
  10,
  0,
  0,
  0,
  0,
};
#endif

//***************************************************************************
// Description: Wakaama accepts an empty descriptor and uses Kconfig defaults.
//***************************************************************************

static void test_proto_wakaama_defaults_configure()
{
  CDescObject desc(g_cfg_wakaama_defaults);
  CProtoWakaama wakaama(desc);

  TEST_ASSERT_EQUAL(OK, wakaama.configure());
  TEST_ASSERT_EQUAL_STRING(CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_MANUFACTURER,
                           wakaama.deviceManufacturer());
  TEST_ASSERT_EQUAL_STRING(CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_MODEL_NUMBER,
                           wakaama.deviceModelNumber());
  TEST_ASSERT_EQUAL_STRING(CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_SERIAL_NUMBER,
                           wakaama.deviceSerialNumber());
  TEST_ASSERT_EQUAL_STRING(CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_FIRMWARE_VERSION,
                           wakaama.deviceFirmwareVersion());
}

//***************************************************************************
// Description: Wakaama configures and initializes legacy plus bootstrap servers.
//***************************************************************************

static void test_proto_wakaama_multiserver_init()
{
  CDescObject desc(g_cfg_wakaama_multiserver);
  CProtoWakaama wakaama(desc);

  TEST_ASSERT_EQUAL(OK, wakaama.configure());
  TEST_ASSERT_EQUAL_STRING("Unit", wakaama.deviceManufacturer());
  TEST_ASSERT_EQUAL_STRING("Model", wakaama.deviceModelNumber());
  TEST_ASSERT_EQUAL_STRING("Ser1", wakaama.deviceSerialNumber());
  TEST_ASSERT_EQUAL_STRING("1.0", wakaama.deviceFirmwareVersion());
  TEST_ASSERT_FALSE(wakaama.hasThread());
  TEST_ASSERT_EQUAL(OK, wakaama.init());
  TEST_ASSERT_FALSE(wakaama.hasThread());
  TEST_ASSERT_EQUAL(OK, wakaama.deinit());
}

//***************************************************************************
// Description: Wakaama rejects PSK security without a pre-shared key.
//***************************************************************************

static void test_proto_wakaama_rejects_psk_without_key()
{
  CDescObject desc(g_cfg_wakaama_psk_missing_key);
  CProtoWakaama wakaama(desc);

  TEST_ASSERT_EQUAL(-EINVAL, wakaama.configure());
}

//***************************************************************************
// Description: Wakaama rejects a descriptor item that mixes object IDs.
//***************************************************************************

static void test_proto_wakaama_rejects_mixed_iobind_object()
{
  CDescObject desc(g_cfg_wakaama_mixed_iobind_object);
  CProtoWakaama wakaama(desc);

  TEST_ASSERT_EQUAL(-EINVAL, wakaama.configure());
}

#ifndef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
//***************************************************************************
// Description: Wakaama rejects coaps servers when DTLS support is disabled.
//***************************************************************************

static void test_proto_wakaama_rejects_coaps_without_dtls()
{
  CDescObject desc(g_cfg_wakaama_coaps_no_dtls);
  CProtoWakaama wakaama(desc);

  TEST_ASSERT_EQUAL(-ENOTSUP, wakaama.configure());
}
#endif

//***************************************************************************
// Description: Server full read allocates only readable resources.
//***************************************************************************

static void test_proto_wakaama_server_full_read_skips_execute_resources()
{
  lwm2m_object_t object;
  InstancePools pools;
  server_instance_s serverPool[1];
  int numData = 0;
  lwm2m_data_t *data = nullptr;

  add_server_instance(object, pools, serverPool, 0);

  TEST_ASSERT_EQUAL(COAP_205_CONTENT, serverRead(nullptr, 0, &numData, &data, &object));
  TEST_ASSERT_NOT_NULL(data);

  TEST_ASSERT_TRUE(data_has_id(data, numData, LWM2M_SERVER_COMM_RETRY_COUNT_ID));
  TEST_ASSERT_TRUE(data_has_id(data, numData, LWM2M_SERVER_PREFERRED_TRANSPORT_ID));
  TEST_ASSERT_TRUE(data_has_id(data, numData, LWM2M_SERVER_MUTE_SEND_ID));
  TEST_ASSERT_FALSE(data_has_id(data, numData, LWM2M_SERVER_UPDATE_ID));
  TEST_ASSERT_FALSE(data_has_id(data, numData, LWM2M_SERVER_DISABLE_ID));

  assert_data_int(data, numData, LWM2M_SERVER_COMM_RETRY_COUNT_ID, 5);
  assert_data_string(data, numData, LWM2M_SERVER_PREFERRED_TRANSPORT_ID, "U");
  assert_data_bool(data, numData, LWM2M_SERVER_MUTE_SEND_ID, false);

  lwm2m_data_free(numData, data);
}

//***************************************************************************
// Description: Server discover reports readable and executable resources.
//***************************************************************************

static void test_proto_wakaama_server_discover_includes_execute_resources()
{
  lwm2m_object_t object;
  InstancePools pools;
  server_instance_s serverPool[1];
  int numData = 0;
  lwm2m_data_t *data = nullptr;

  add_server_instance(object, pools, serverPool, 0);

  TEST_ASSERT_EQUAL(COAP_205_CONTENT, serverDiscover(nullptr, 0, &numData, &data, &object));
  TEST_ASSERT_NOT_NULL(data);

  TEST_ASSERT_TRUE(data_has_id(data, numData, LWM2M_SERVER_UPDATE_ID));
  TEST_ASSERT_TRUE(data_has_id(data, numData, LWM2M_SERVER_DISABLE_ID));
  TEST_ASSERT_TRUE(data_has_id(data, numData, LWM2M_SERVER_COMM_RETRY_COUNT_ID));
  TEST_ASSERT_TRUE(data_has_id(data, numData, LWM2M_SERVER_PREFERRED_TRANSPORT_ID));

  lwm2m_data_free(numData, data);
}

//***************************************************************************
// Description: Server write updates the LwM2M 1.1 readable resources.
//***************************************************************************

static void test_proto_wakaama_server_write_updates_extended_resources()
{
  lwm2m_object_t object;
  InstancePools pools;
  server_instance_s serverPool[1];
  int numWrite = 4;
  lwm2m_data_t *writeData = lwm2m_data_new(numWrite);

  add_server_instance(object, pools, serverPool, 0);
  TEST_ASSERT_NOT_NULL(writeData);

  writeData[0].id = LWM2M_SERVER_MIN_PERIOD_ID;
  lwm2m_data_encode_int(3, &writeData[0]);
  writeData[1].id = LWM2M_SERVER_COMM_RETRY_COUNT_ID;
  lwm2m_data_encode_int(7, &writeData[1]);
  writeData[2].id = LWM2M_SERVER_MUTE_SEND_ID;
  lwm2m_data_encode_bool(true, &writeData[2]);
  writeData[3].id = LWM2M_SERVER_REG_FAIL_BOOTSTRAP_ID;
  lwm2m_data_encode_bool(false, &writeData[3]);

  TEST_ASSERT_EQUAL(
    COAP_204_CHANGED,
    serverWrite(nullptr, 0, numWrite, writeData, &object, LWM2M_WRITE_REPLACE_RESOURCES));

  lwm2m_data_free(numWrite, writeData);

  int numRead = 4;
  lwm2m_data_t *readData = lwm2m_data_new(numRead);

  TEST_ASSERT_NOT_NULL(readData);
  readData[0].id = LWM2M_SERVER_MIN_PERIOD_ID;
  readData[1].id = LWM2M_SERVER_COMM_RETRY_COUNT_ID;
  readData[2].id = LWM2M_SERVER_MUTE_SEND_ID;
  readData[3].id = LWM2M_SERVER_REG_FAIL_BOOTSTRAP_ID;

  TEST_ASSERT_EQUAL(COAP_205_CONTENT, serverRead(nullptr, 0, &numRead, &readData, &object));
  assert_data_int(readData, numRead, LWM2M_SERVER_MIN_PERIOD_ID, 3);
  assert_data_int(readData, numRead, LWM2M_SERVER_COMM_RETRY_COUNT_ID, 7);
  assert_data_bool(readData, numRead, LWM2M_SERVER_MUTE_SEND_ID, true);
  assert_data_bool(readData, numRead, LWM2M_SERVER_REG_FAIL_BOOTSTRAP_ID, false);

  lwm2m_data_free(numRead, readData);
}

//***************************************************************************
// Description: Server write maps bad payloads and execute-only writes to errors.
//***************************************************************************

static void test_proto_wakaama_server_write_rejects_bad_values()
{
  lwm2m_object_t object;
  InstancePools pools;
  server_instance_s serverPool[1];
  lwm2m_data_t data;

  add_server_instance(object, pools, serverPool, 0);

  std::memset(&data, 0, sizeof(data));
  data.id = LWM2M_SERVER_COMM_RETRY_COUNT_ID;
  lwm2m_data_encode_int(300, &data);

  TEST_ASSERT_EQUAL(COAP_400_BAD_REQUEST,
                    serverWrite(nullptr, 0, 1, &data, &object, LWM2M_WRITE_REPLACE_RESOURCES));

  std::memset(&data, 0, sizeof(data));
  data.id = LWM2M_SERVER_UPDATE_ID;
  lwm2m_data_encode_int(0, &data);

  TEST_ASSERT_EQUAL(COAP_405_METHOD_NOT_ALLOWED,
                    serverWrite(nullptr, 0, 1, &data, &object, LWM2M_WRITE_REPLACE_RESOURCES));
}

extern "C"
{
  int test_proto_wakaama()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_proto_wakaama_defaults_configure);
    DAWN_RUN_TEST(test_proto_wakaama_multiserver_init);
    DAWN_RUN_TEST(test_proto_wakaama_rejects_psk_without_key);
    DAWN_RUN_TEST(test_proto_wakaama_rejects_mixed_iobind_object);
#ifndef CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK
    DAWN_RUN_TEST(test_proto_wakaama_rejects_coaps_without_dtls);
#endif
    DAWN_RUN_TEST(test_proto_wakaama_server_full_read_skips_execute_resources);
    DAWN_RUN_TEST(test_proto_wakaama_server_discover_includes_execute_resources);
    DAWN_RUN_TEST(test_proto_wakaama_server_write_updates_extended_resources);
    DAWN_RUN_TEST(test_proto_wakaama_server_write_rejects_bad_values);

    return UNITY_END();
  }
}
