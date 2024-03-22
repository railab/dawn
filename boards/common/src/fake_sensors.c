/*
 * boards/common/src/fake_sensors.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

#include <nuttx/config.h>

#include <debug.h>

#include <nuttx/sensors/sensor.h>

#include "dawn/fakesensor2.h"

/****************************************************************************
 * Private data
 ****************************************************************************/

/* Fake accel1 */

const float g_fake_fakesensor_accel1_data[] =
{
  /* X, Y, Z, temp */

  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
};

const struct fakesensor2_data_s g_fake_fakesensor_accel1 =
{
  .interval = 1000,
  .dlen = sizeof(g_fake_fakesensor_accel1_data) / sizeof(float),
  .data = g_fake_fakesensor_accel1_data
};

const float g_fake_fakesensor_accel2_data[] =
{
  /* X, Y, Z, temp */

  -1.0f, -2.0f, -3.0f, 0.0f,
  -1.0f, -2.0f, -3.0f, 0.0f,
  -1.0f, -2.0f, -3.0f, 0.0f,
  -1.0f, -2.0f, -3.0f, 0.0f,
  -1.0f, -2.0f, -3.0f, 0.0f,
  -1.0f, -2.0f, -3.0f, 0.0f,
  -1.0f, -2.0f, -3.0f, 0.0f,
  -1.0f, -2.0f, -3.0f, 0.0f,
  -1.0f, -2.0f, -3.0f, 0.0f,
  -1.0f, -2.0f, -3.0f, 0.0f,
};

const struct fakesensor2_data_s g_fake_fakesensor_accel2 =
{
  .interval = 1000,
  .dlen = sizeof(g_fake_fakesensor_accel2_data) / sizeof(float),
  .data = g_fake_fakesensor_accel2_data
};

/* Fake mag */

const float g_fake_fakesensor_mag_data[] =
{
  /* X, Y, Z, temp, status */

  1.0f, 2.0f, 3.0f, 0.0f, 0,
  1.0f, 2.0f, 3.0f, 0.0f, 0,
  1.0f, 2.0f, 3.0f, 0.0f, 0,
  1.0f, 2.0f, 3.0f, 0.0f, 0,
  1.0f, 2.0f, 3.0f, 0.0f, 0,
  1.0f, 2.0f, 3.0f, 0.0f, 0,
  1.0f, 2.0f, 3.0f, 0.0f, 0,
  1.0f, 2.0f, 3.0f, 0.0f, 0,
  1.0f, 2.0f, 3.0f, 0.0f, 0,
  1.0f, 2.0f, 3.0f, 0.0f, 0,
};

const struct fakesensor2_data_s g_fake_fakesensor_mag =
{
  .interval = 1000,
  .dlen = sizeof(g_fake_fakesensor_mag_data) / sizeof(float),
  .data = g_fake_fakesensor_mag_data
};

/* Fake gyro */

const float g_fake_fakesensor_gyro_data[] =
{
  /* X, Y, Z, temp */

  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
  1.0f, 2.0f, 3.0f, 0.0f,
};

const struct fakesensor2_data_s g_fake_fakesensor_gyro =
{
  .interval = 1000,
  .dlen = sizeof(g_fake_fakesensor_gyro_data) / sizeof(float),
  .data = g_fake_fakesensor_gyro_data
};

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: board_late_init
 *
 * Description:
 *   Initialzie fake sensors for Dawn.
 *
 ****************************************************************************/

int fake_sensors_initialize(void)
{
  int ret = OK;

  /* Fake accel1 */

  ret = fakesensor2_init(SENSOR_TYPE_ACCELEROMETER,
                         &g_fake_fakesensor_accel1, 0, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize accel %d\n", ret);
      goto errout;
    }

  /* Fake accel2 */

  ret = fakesensor2_init(SENSOR_TYPE_ACCELEROMETER,
                         &g_fake_fakesensor_accel2, 1, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize accel %d\n", ret);
      goto errout;
    }

  /* Fake mag */

  ret = fakesensor2_init(SENSOR_TYPE_MAGNETIC_FIELD,
                         &g_fake_fakesensor_mag, 0, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize mag %d\n", ret);
      goto errout;
    }

  /* Fake gyro */

  ret = fakesensor2_init(SENSOR_TYPE_GYROSCOPE,
                         &g_fake_fakesensor_gyro, 0, 0);
  if (ret < 0)
    {
      syslog(LOG_ERR, "ERROR: Failed to initialize gyro %d\n", ret);
      goto errout;
    }

errout:
  return ret;
}
