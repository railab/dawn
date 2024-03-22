// dawn/tests/io/test_sensor.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/sensor.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_sensor_accel1[] = {
  CIOSensor::objectIdAccel(SObjectId::DTYPE_FLOAT, false, 1),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

static uint32_t g_cfg_sensor_accel2[] = {
  CIOSensor::objectIdAccel(SObjectId::DTYPE_FLOAT, true, 2),
  1,
  CIOCommon::cfgIdDevno(),
  1,
};

static uint32_t g_cfg_sensor_accel_b16[] = {
  CIOSensor::objectIdAccel(SObjectId::DTYPE_B16, false, 1),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

//***************************************************************************
// Description: sensor IO reports a getDataSize() that matches a 4-element
// float buffer.
//***************************************************************************

static void test_io_sensor_data_size()
{
  CDescObject desc(g_cfg_sensor_accel1);
  CIOSensor sensor(desc);
  io_sdata_t<float, 4> data;

  TEST_ASSERT_EQUAL(OK, sensor.configure());
  TEST_ASSERT_EQUAL(OK, sensor.init());

  TEST_ASSERT_EQUAL(data.getDataSize(), sensor.getDataSize());
}

//***************************************************************************
// Description: b16 sensor descriptors are accepted only when the NuttX sensor
// framework itself is configured to publish b16_t samples.
//***************************************************************************

static void test_io_sensor_b16_requires_nuttx_b16()
{
  CDescObject desc(g_cfg_sensor_accel_b16);
  CIOSensor sensor(desc);

#ifdef CONFIG_SENSORS_USE_B16
  TEST_ASSERT_EQUAL(OK, sensor.configure());
#else
  TEST_ASSERT_EQUAL(-EINVAL, sensor.configure());
#endif
}

//***************************************************************************
// Description: setData on a read-only sensor IO returns -ENOTSUP.
//***************************************************************************

static void test_io_sensor_set_unsupported()
{
  CDescObject desc(g_cfg_sensor_accel1);
  CIOSensor sensor(desc);
  io_sdata_t<float, 4> data;

  TEST_ASSERT_EQUAL(OK, sensor.configure());
  TEST_ASSERT_EQUAL(OK, sensor.init());

  TEST_ASSERT_EQUAL(-ENOTSUP, sensor.setData(data));
}

//***************************************************************************
// Description: a non-timestamped accelerometer returns the mock's positive
// xyz triplet (1, 2, 3) and ts == 0.
//***************************************************************************

static void test_io_sensor_accel_no_ts()
{
  CDescObject desc(g_cfg_sensor_accel1);
  CIOSensor sensor(desc);
  io_sdata_t<float, 4> data;

  TEST_ASSERT_EQUAL(OK, sensor.configure());
  TEST_ASSERT_EQUAL(OK, sensor.init());

  sleep(1);

  TEST_ASSERT_EQUAL(0, sensor.getData(data, 1));
  TEST_ASSERT_EQUAL_UINT64(0, data[0]);
  TEST_ASSERT_EQUAL_FLOAT(1.0f, data(0));
  TEST_ASSERT_EQUAL_FLOAT(2.0f, data(1));
  TEST_ASSERT_EQUAL_FLOAT(3.0f, data(2));
}

//***************************************************************************
// Description: a timestamped accelerometer returns the mock's negative xyz
// triplet (-1, -2, -3) and a non-zero timestamp.
//***************************************************************************

static void test_io_sensor_accel_with_ts()
{
  CDescObject desc(g_cfg_sensor_accel2);
  CIOSensor sensor(desc);
  io_sdata_t<float, 4> data;

  TEST_ASSERT_EQUAL(OK, sensor.configure());
  TEST_ASSERT_EQUAL(OK, sensor.init());

  sleep(1);

  TEST_ASSERT_EQUAL(0, sensor.getData(data, 1));
  TEST_ASSERT(data[0] > 0);
  TEST_ASSERT_EQUAL_FLOAT(-1.0f, data(0));
  TEST_ASSERT_EQUAL_FLOAT(-2.0f, data(1));
  TEST_ASSERT_EQUAL_FLOAT(-3.0f, data(2));
}

//***************************************************************************
// Description: sensor IO does not support batched reads.
//***************************************************************************

static void test_io_sensor_batch_unsupported()
{
  CDescObject desc(g_cfg_sensor_accel1);
  CIOSensor sensor(desc);
  io_sdata_t<float, 4> data;

  TEST_ASSERT_EQUAL(OK, sensor.configure());
  TEST_ASSERT_EQUAL(OK, sensor.init());

  TEST_ASSERT_FALSE(sensor.isBatch());
  TEST_ASSERT_EQUAL(-ENOTSUP, sensor.getData(data, 2));
}

extern "C"
{
  int test_io_sensor()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_sensor_data_size);
    DAWN_RUN_TEST(test_io_sensor_b16_requires_nuttx_b16);
    DAWN_RUN_TEST(test_io_sensor_set_unsupported);
    DAWN_RUN_TEST(test_io_sensor_accel_no_ts);
    DAWN_RUN_TEST(test_io_sensor_accel_with_ts);
    DAWN_RUN_TEST(test_io_sensor_batch_unsupported);

    return UNITY_END();
  }
}
