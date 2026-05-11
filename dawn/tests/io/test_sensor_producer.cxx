// dawn/tests/io/test_sensor_producer.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/factory.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/io/sensor_producer.hxx"
#include "test_common.hxx"

#include <cerrno>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <nuttx/sensors/sensor.h>

using namespace dawn;

static constexpr int SENSOR_PRODUCER_TEMP_DEVNO = 41;
static constexpr const char *SENSOR_PRODUCER_TEMP_PATH = "/dev/uorb/sensor_temp41";

static uint32_t g_cfg_sensor_producer_temp[] = {
  CIOSensorProducer::objectIdTemp(SObjectId::DTYPE_FLOAT, false, 0),
  3,
  CIOCommon::cfgIdDevno(),
  SENSOR_PRODUCER_TEMP_DEVNO,
  CIOSensorProducer::cfgIdQueueSize(),
  4,
  CIOSensorProducer::cfgIdPersist(),
  false,
};

static uint32_t g_cfg_sensor_producer_temp_bad_dtype[] = {
  CIOSensorProducer::objectIdTemp(SObjectId::DTYPE_UINT32, false, 1),
  1,
  CIOCommon::cfgIdDevno(),
  42,
};

static int read_temp_sample(int fd, struct sensor_temp *sample)
{
  struct pollfd pfd;
  ssize_t nread;
  int ret;

  pfd.fd = fd;
  pfd.events = POLLIN;
  pfd.revents = 0;

  ret = poll(&pfd, 1, 1000);
  if (ret <= 0)
    {
      return ret == 0 ? -ETIMEDOUT : -errno;
    }

  nread = read(fd, sample, sizeof(*sample));
  if (nread < 0)
    {
      return -errno;
    }

  return nread == sizeof(*sample) ? OK : -EIO;
}

//***************************************************************************
// Description: sensor producer exposes a write-only scalar temperature IO.
//***************************************************************************

static void test_io_sensor_producer_contract()
{
  CDescObject desc(g_cfg_sensor_producer_temp);
  CIOSensorProducer producer(desc);
  io_sdata_t<float, 1> data;

  TEST_ASSERT_FALSE(producer.isRead());
  TEST_ASSERT_TRUE(producer.isWrite());
  TEST_ASSERT_FALSE(producer.isNotify());
  TEST_ASSERT_FALSE(producer.isBatch());
  TEST_ASSERT_EQUAL(sizeof(sensor_data_t), producer.getDataSize());
  TEST_ASSERT_EQUAL(1u, producer.getDataDim());
  TEST_ASSERT_EQUAL(-ENODEV, producer.setData(data));
  TEST_ASSERT_EQUAL(-ENOTSUP, producer.getData(data, 1));
}

//***************************************************************************
// Description: factory creates the concrete producer for producer object IDs.
//***************************************************************************

static void test_io_sensor_producer_factory_create()
{
  CIOFactory factory;
  CDescObject desc(g_cfg_sensor_producer_temp);
  CIOCommon *io;

  io = factory.create(desc);
  TEST_ASSERT_NOT_NULL(io);
  TEST_ASSERT_FALSE(io->isRead());
  TEST_ASSERT_TRUE(io->isWrite());

  delete io;
}

//***************************************************************************
// Description: producer rejects descriptors whose dtype does not match the
// NuttX sensor framework sample type.
//***************************************************************************

static void test_io_sensor_producer_rejects_bad_dtype()
{
  CDescObject desc(g_cfg_sensor_producer_temp_bad_dtype);
  CIOSensorProducer producer(desc);

  TEST_ASSERT_EQUAL(-EINVAL, producer.configure());
}

//***************************************************************************
// Description: setData validates protocol payload size before publishing.
//***************************************************************************

static void test_io_sensor_producer_rejects_wrong_payload_size()
{
  CDescObject desc(g_cfg_sensor_producer_temp);
  CIOSensorProducer producer(desc);
  io_sdata_t<float, 2> data;

  TEST_ASSERT_EQUAL(OK, producer.configure());
  TEST_ASSERT_EQUAL(OK, producer.init());
  TEST_ASSERT_EQUAL(-EINVAL, producer.setData(data));
  TEST_ASSERT_EQUAL(OK, producer.deinit());
}

//***************************************************************************
// Description: multiple setData calls publish distinct NuttX sensor events
// that an independent reader can consume from the uORB topic.
//***************************************************************************

static void test_io_sensor_producer_publishes_temp_updates()
{
  CDescObject desc(g_cfg_sensor_producer_temp);
  CIOSensorProducer producer(desc);
  io_sdata_t<float, 1> data;
  struct sensor_temp sample;
  uint64_t last_ts = 0;
  int fd;

  static constexpr float values[] = {23.5f, 24.25f, 19.75f};

  TEST_ASSERT_EQUAL(OK, producer.configure());
  TEST_ASSERT_EQUAL(OK, producer.init());

  fd = open(SENSOR_PRODUCER_TEMP_PATH, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
  TEST_ASSERT(fd >= 0);

  for (size_t i = 0; i < sizeof(values) / sizeof(values[0]); i++)
    {
      data(0) = values[i];

      TEST_ASSERT_EQUAL(OK, producer.setData(data));
      TEST_ASSERT_EQUAL(OK, read_temp_sample(fd, &sample));
      TEST_ASSERT_FLOAT_WITHIN(0.001f, values[i], static_cast<float>(sample.temperature));
      TEST_ASSERT(sample.timestamp >= last_ts);
      last_ts = sample.timestamp;
    }

  close(fd);
  TEST_ASSERT_EQUAL(OK, producer.deinit());
}

extern "C"
{
  int test_io_sensor_producer()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_sensor_producer_contract);
    DAWN_RUN_TEST(test_io_sensor_producer_factory_create);
    DAWN_RUN_TEST(test_io_sensor_producer_rejects_bad_dtype);
    DAWN_RUN_TEST(test_io_sensor_producer_rejects_wrong_payload_size);
    DAWN_RUN_TEST(test_io_sensor_producer_publishes_temp_updates);

    return UNITY_END();
  }
}
