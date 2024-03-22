/*
 * boards/common/src/fakesensor2_uorb.c
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 */

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/param.h>

#include <nuttx/fs/fs.h>
#include <nuttx/kmalloc.h>
#include <nuttx/kthread.h>
#include <nuttx/nuttx.h>
#include <nuttx/semaphore.h>
#include <nuttx/sensors/sensor.h>
#include <nuttx/sensors/gnss.h>
#include <nuttx/signal.h>
#include <debug.h>

#include "dawn/fakesensor2.h"

/****************************************************************************
 * Private Types
 ****************************************************************************/

struct fakesensor2_s
{
  union
    {
      struct sensor_lowerhalf_s lower;
      struct gnss_lowerhalf_s gnss;
    };

  int type;
  uint32_t interval;
  uint32_t batch;
  int raw_start;
  FAR const struct fakesensor2_data_s *samples;
  sem_t wakeup;
  volatile bool running;
};

/****************************************************************************
 * Private Function Prototypes
 ****************************************************************************/

static int fakesensor2_activate(FAR struct sensor_lowerhalf_s *lower,
                                FAR struct file *filep, bool enable);
static int fakesensor2_set_interval(FAR struct sensor_lowerhalf_s *lower,
                                    FAR struct file *filep,
                                    FAR uint32_t *period_us);
static int fakesensor2_batch(FAR struct sensor_lowerhalf_s *lower,
                             FAR struct file *filep,
                             FAR uint32_t *latency_us);
static int fakegnss_activate(FAR struct gnss_lowerhalf_s *lower,
                             FAR struct file *filep, bool sw);
static int fakegnss_set_interval(FAR struct gnss_lowerhalf_s *lower,
                                 FAR struct file *filep,
                                 FAR uint32_t *period_us);
static void fakesensor2_push_event(FAR struct fakesensor2_s *sensor,
                                   uint64_t event_timestamp);
static int fakesensor2_thread(int argc, char** argv);

/****************************************************************************
 * Private Data
 ****************************************************************************/

static struct sensor_ops_s g_fakesensor2_ops =
{
  .activate = fakesensor2_activate,
  .set_interval = fakesensor2_set_interval,
  .batch = fakesensor2_batch,
};

static struct gnss_ops_s g_fakegnss_ops =
{
  .activate = fakegnss_activate,
  .set_interval = fakegnss_set_interval,
};

/****************************************************************************
 * Private Functions
 ****************************************************************************/

static inline void fakesensor2_read_accel(FAR struct fakesensor2_s *sensor,
                                          uint64_t event_timestamp)
{
  struct sensor_accel accel;

  accel.x = sensor->samples->data[sensor->raw_start++];
  accel.y = sensor->samples->data[sensor->raw_start++];
  accel.z = sensor->samples->data[sensor->raw_start++];
  accel.temperature = sensor->samples->data[sensor->raw_start++];

  accel.timestamp = event_timestamp;
  sensor->lower.push_event(sensor->lower.priv, &accel,
                           sizeof(struct sensor_accel));
}

static inline void fakesensor2_read_mag(FAR struct fakesensor2_s *sensor,
                                        uint64_t event_timestamp)
{
  struct sensor_mag mag;

  mag.x = sensor->samples->data[sensor->raw_start++];
  mag.y = sensor->samples->data[sensor->raw_start++];
  mag.z = sensor->samples->data[sensor->raw_start++];
  mag.temperature = sensor->samples->data[sensor->raw_start++];
  mag.status = sensor->samples->data[sensor->raw_start++];

  mag.timestamp = event_timestamp;
  sensor->lower.push_event(sensor->lower.priv, &mag,
                           sizeof(struct sensor_mag));
}

static inline void fakesensor2_read_gyro(FAR struct fakesensor2_s *sensor,
                                         uint64_t event_timestamp)
{
  struct sensor_gyro gyro;

  gyro.x = sensor->samples->data[sensor->raw_start++];
  gyro.y = sensor->samples->data[sensor->raw_start++];
  gyro.z = sensor->samples->data[sensor->raw_start++];
  gyro.temperature = sensor->samples->data[sensor->raw_start++];

  gyro.timestamp = event_timestamp;
  sensor->lower.push_event(sensor->lower.priv, &gyro,
                           sizeof(struct sensor_gyro));
}

static inline void fakesensor2_read_gnss(FAR struct fakesensor2_s *sensor,
                                         uint64_t event_timestamp)
{
  FAR const float *data = sensor->samples->data;
  struct sensor_gnss gnss;

  gnss.time_utc = *((uint64_t *)&data[sensor->raw_start++]);
  sensor->raw_start++;

  gnss.latitude           = data[sensor->raw_start++];
  gnss.longitude          = data[sensor->raw_start++];
  gnss.altitude           = data[sensor->raw_start++];
  gnss.altitude_ellipsoid = data[sensor->raw_start++];
  gnss.eph                = data[sensor->raw_start++];
  gnss.epv                = data[sensor->raw_start++];
  gnss.hdop               = data[sensor->raw_start++];
  gnss.pdop               = data[sensor->raw_start++];
  gnss.vdop               = data[sensor->raw_start++];
  gnss.ground_speed       = data[sensor->raw_start++];

  gnss.satellites_used = *((uint32_t *)&data[sensor->raw_start++]);
  gnss.firmware_ver    = *((uint32_t *)&data[sensor->raw_start++]);

  gnss.timestamp = event_timestamp;
  sensor->lower.push_event(sensor->lower.priv, &gnss,
                           sizeof(struct sensor_gnss));
}

static int fakesensor2_activate(FAR struct sensor_lowerhalf_s *lower,
                                FAR struct file *filep, bool enable)
{
  FAR struct fakesensor2_s *sensor =
    container_of(lower, struct fakesensor2_s, lower);

  if (enable)
    {
      sensor->running = true;

      /* Wake up the thread */

      nxsem_post(&sensor->wakeup);
    }
  else
    {
      sensor->running = false;
    }

  return OK;
}

static int fakegnss_activate(FAR struct gnss_lowerhalf_s *lower,
                             FAR struct file *filep, bool enable)
{
  return fakesensor2_activate((FAR void *)lower, filep, enable);
}

static int fakesensor2_set_interval(FAR struct sensor_lowerhalf_s *lower,
                                    FAR struct file *filep,
                                    FAR uint32_t *period_us)
{
  FAR struct fakesensor2_s *sensor =
    container_of(lower, struct fakesensor2_s, lower);

  sensor->interval = *period_us;
  return OK;
}

static int fakegnss_set_interval(FAR struct gnss_lowerhalf_s *lower,
                                 FAR struct file *filep,
                                 FAR uint32_t *period_us)
{
  return fakesensor2_set_interval((FAR void *)lower, filep, period_us);
}

static int fakesensor2_batch(FAR struct sensor_lowerhalf_s *lower,
                             FAR struct file *filep,
                             FAR uint32_t *latency_us)
{
  FAR struct fakesensor2_s *sensor =
    container_of(lower, struct fakesensor2_s, lower);
  uint32_t max_latency = sensor->lower.nbuffer * sensor->interval;

  if (*latency_us > max_latency)
    {
      *latency_us = max_latency;
    }
  else if (*latency_us < sensor->interval && *latency_us > 0)
    {
      *latency_us = sensor->interval;
    }

  sensor->batch = *latency_us;
  return OK;
}

static void fakesensor2_push_event(FAR struct fakesensor2_s *sensor,
                                   uint64_t event_timestamp)
{
  switch (sensor->type)
  {
    case SENSOR_TYPE_ACCELEROMETER:
      fakesensor2_read_accel(sensor, event_timestamp);
      break;

    case SENSOR_TYPE_MAGNETIC_FIELD:
      fakesensor2_read_mag(sensor, event_timestamp);
      break;

    case SENSOR_TYPE_GYROSCOPE:
      fakesensor2_read_gyro(sensor, event_timestamp);
      break;

    case SENSOR_TYPE_GNSS:
    case SENSOR_TYPE_GNSS_SATELLITE:
      fakesensor2_read_gnss(sensor, event_timestamp);
      break;

    default:
      snerr("fakesensor2: unsupported sensor type\n");
      break;
  }
}

static int fakesensor2_thread(int argc, char** argv)
{
  FAR struct fakesensor2_s *sensor = (FAR struct fakesensor2_s *)
    ((uintptr_t)strtoul(argv[1], NULL, 16));

  /* Waiting to be woken up */

  nxsem_wait_uninterruptible(&sensor->wakeup);

  sensor->interval = sensor->samples->interval;

  while (sensor->running)
    {
      uint32_t sleep_us = sensor->batch ? sensor->batch : sensor->interval;

      /* Sleeping thread for interval */

      nxsig_usleep(sleep_us);

      /* Notify upper */

      if (sensor->batch)
        {
          uint32_t batch_num = sensor->batch / sensor->interval;
          uint64_t event_timestamp =
            sensor_get_timestamp() - sensor->interval * batch_num;
          int i;

          for (i = 0; i < batch_num; i++)
            {
              fakesensor2_push_event(sensor, event_timestamp);
              event_timestamp += sensor->interval;
            }
        }
      else
        {
          fakesensor2_push_event(sensor, sensor_get_timestamp());
        }

      if (sensor->raw_start >= sensor->samples->dlen)
        {
          sensor->raw_start = 0;
        }
    }

  return OK;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * Name: fakesensor2_init
 *
 * Description:
 *   This function generates a sensor node under /dev/uorb/ and reports
 *   data from a provided in-memory samples array.
 *
 * Input Parameters:
 *   type         - The type of sensor defined in <nuttx/sensors/sensor.h>
 *   samples      - Sensor data provided as structure with data array
 *   devno        - The user specifies which device of this type, from 0.
 *   batch_number - The maximum number of batch
 *
 ****************************************************************************/

int fakesensor2_init(int type, FAR const struct fakesensor2_data_s *samples,
                     int devno, uint32_t batch_number)
{
  FAR struct fakesensor2_s *sensor;
  FAR char *argv[2];
  char arg1[32];
  int ret;
  uint32_t nbuffer[] = {
    [SENSOR_GNSS_IDX_GNSS] = batch_number,
    [SENSOR_GNSS_IDX_GNSS_SATELLITE] = batch_number,
    [SENSOR_GNSS_IDX_GNSS_MEASUREMENT] = batch_number,
    [SENSOR_GNSS_IDX_GNSS_CLOCK] = batch_number,
    [SENSOR_GNSS_IDX_GNSS_GEOFENCE] = batch_number,
  };

  /* Alloc memory for sensor */

  sensor = kmm_zalloc(sizeof(struct fakesensor2_s));
  if (!sensor)
    {
      snerr("Memory cannot be allocated for fakesensor2\n");
      return -ENOMEM;
    }

  sensor->samples = samples;
  sensor->type = type;

  nxsem_init(&sensor->wakeup, 0, 0);

  /* Create thread for sensor */

  snprintf(arg1, 32, "%p", sensor);
  argv[0] = arg1;
  argv[1] = NULL;
  ret = kthread_create("fakesensor2_thread", SCHED_PRIORITY_DEFAULT,
                       CONFIG_DEFAULT_TASK_STACKSIZE,
                       fakesensor2_thread, argv);
  if (ret < 0)
    {
      kmm_free(sensor);
      return ERROR;
    }

  /* Register sensor */

  if (type == SENSOR_TYPE_GNSS || type == SENSOR_TYPE_GNSS_SATELLITE)
    {
      sensor->gnss.ops = &g_fakegnss_ops;
      gnss_register(&sensor->gnss, devno, nbuffer, nitems(nbuffer));
    }
  else
    {
      sensor->lower.type = type;
      sensor->lower.ops = &g_fakesensor2_ops;
      sensor->lower.nbuffer = batch_number;
      sensor_register(&sensor->lower, devno);
    }

  return OK;
}
