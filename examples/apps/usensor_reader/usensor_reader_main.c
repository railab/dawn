// examples/apps/usensor_reader/usensor_reader_main.c
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <poll.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <nuttx/config.h>
#include <fixedmath.h>
#include <nuttx/sensors/sensor.h>

#define DEFAULT_SENSOR_PATH     "/dev/uorb/sensor_temp10"
#define DEFAULT_READ_COUNT      1
#define DEFAULT_POLL_TIMEOUT_MS 5000
#define OPEN_RETRY_USEC         100000
#define OPEN_RETRY_COUNT        300

enum sensor_reader_kind_e
{
  SENSOR_READER_TEMP,
  SENSOR_READER_HUMI,
  SENSOR_READER_BARO,
  SENSOR_READER_LIGHT,
};

union sensor_reader_sample_u
{
  struct sensor_temp temp;
  struct sensor_humi humi;
  struct sensor_baro baro;
  struct sensor_light light;
};

static int open_with_retry(FAR const char *path)
{
  int fd;

  for (int i = 0; i < OPEN_RETRY_COUNT; i++)
    {
      fd = open(path, O_RDONLY | O_CLOEXEC);
      if (fd >= 0)
        {
          return fd;
        }

      usleep(OPEN_RETRY_USEC);
    }

  return -errno;
}

static enum sensor_reader_kind_e sensor_kind_from_path(FAR const char *path)
{
  if (strstr(path, "sensor_humi") != NULL)
    {
      return SENSOR_READER_HUMI;
    }

  if (strstr(path, "sensor_baro") != NULL)
    {
      return SENSOR_READER_BARO;
    }

  if (strstr(path, "sensor_light") != NULL)
    {
      return SENSOR_READER_LIGHT;
    }

  return SENSOR_READER_TEMP;
}

static size_t sensor_sample_size(enum sensor_reader_kind_e kind)
{
  switch (kind)
    {
      case SENSOR_READER_HUMI:
        return sizeof(struct sensor_humi);
      case SENSOR_READER_BARO:
        return sizeof(struct sensor_baro);
      case SENSOR_READER_LIGHT:
        return sizeof(struct sensor_light);
      case SENSOR_READER_TEMP:
      default:
        return sizeof(struct sensor_temp);
    }
}

static double sensor_value_to_double(sensor_data_t value)
{
#ifdef CONFIG_SENSORS_USE_B16
  return b16tof(value);
#else
  return (double)value;
#endif
}

static void print_usage(FAR const char *progname)
{
  printf("Usage: %s [path] [count] [poll_timeout_ms]\n", progname);
  printf("\n");
  printf("Read NuttX user-sensor samples and print them to the console.\n");
  printf("\n");
  printf("Arguments:\n");
  printf("  path             Sensor topic path. Default: %s\n", DEFAULT_SENSOR_PATH);
  printf("  count            Number of samples to read. Default: %d\n", DEFAULT_READ_COUNT);
  printf("  poll_timeout_ms  Timeout per sample in milliseconds. Default: %d\n",
         DEFAULT_POLL_TIMEOUT_MS);
  printf("\n");
  printf("Supported path families:\n");
  printf("  /dev/uorb/sensor_tempN   temperature\n");
  printf("  /dev/uorb/sensor_humiN   humidity\n");
  printf("  /dev/uorb/sensor_baroN   pressure, temperature\n");
  printf("  /dev/uorb/sensor_lightN  light, infrared\n");
  printf("\n");
  printf("Example:\n");
  printf("  %s /dev/uorb/sensor_temp10 10 60000\n", progname);
}

static void print_sensor_sample(enum sensor_reader_kind_e kind,
                                FAR const char *path,
                                FAR const union sensor_reader_sample_u *sample)
{
  switch (kind)
    {
      case SENSOR_READER_HUMI:
        printf("usensor_reader humi=%.6f timestamp=%" PRIu64 " path=%s\n",
               sensor_value_to_double(sample->humi.humidity),
               sample->humi.timestamp,
               path);
        break;

      case SENSOR_READER_BARO:
        printf("usensor_reader baro=%.6f,%.6f timestamp=%" PRIu64 " path=%s\n",
               sensor_value_to_double(sample->baro.pressure),
               sensor_value_to_double(sample->baro.temperature),
               sample->baro.timestamp,
               path);
        break;

      case SENSOR_READER_LIGHT:
        printf("usensor_reader light=%.6f,%.6f timestamp=%" PRIu64 " path=%s\n",
               sensor_value_to_double(sample->light.light),
               sensor_value_to_double(sample->light.ir),
               sample->light.timestamp,
               path);
        break;

      case SENSOR_READER_TEMP:
      default:
        printf("usensor_reader temp=%.6f timestamp=%" PRIu64 " path=%s\n",
               sensor_value_to_double(sample->temp.temperature),
               sample->temp.timestamp,
               path);
        break;
    }
}

// usensor_reader_main

int main(int argc, FAR char *argv[])
{
  FAR const char *path = DEFAULT_SENSOR_PATH;
  enum sensor_reader_kind_e kind;
  int count = DEFAULT_READ_COUNT;
  int poll_timeout_ms = DEFAULT_POLL_TIMEOUT_MS;
  size_t sample_size;
  struct pollfd pfd;
  int fd;

  if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0))
    {
      print_usage(argv[0]);
      return EXIT_SUCCESS;
    }

  if (argc > 1)
    {
      path = argv[1];
    }

  if (argc > 2)
    {
      count = atoi(argv[2]);
      if (count <= 0)
        {
          count = DEFAULT_READ_COUNT;
        }
    }

  if (argc > 3)
    {
      poll_timeout_ms = atoi(argv[3]);
      if (poll_timeout_ms <= 0)
        {
          poll_timeout_ms = DEFAULT_POLL_TIMEOUT_MS;
        }
    }

  kind = sensor_kind_from_path(path);
  sample_size = sensor_sample_size(kind);

  fd = open_with_retry(path);
  if (fd < 0)
    {
      printf("usensor_reader open failed path=%s err=%d\n", path, fd);
      return EXIT_FAILURE;
    }

  memset(&pfd, 0, sizeof(pfd));
  pfd.fd = fd;
  pfd.events = POLLIN;

  for (int i = 0; i < count; i++)
    {
      union sensor_reader_sample_u sample;
      ssize_t nread;
      int ret;

      ret = poll(&pfd, 1, poll_timeout_ms);
      if (ret < 0)
        {
          printf("usensor_reader poll failed err=%d\n", -errno);
          close(fd);
          return EXIT_FAILURE;
        }

      if (ret == 0)
        {
          printf("usensor_reader timeout path=%s\n", path);
          close(fd);
          return EXIT_FAILURE;
        }

      nread = read(fd, &sample, sample_size);
      if (nread < 0)
        {
          printf("usensor_reader read failed err=%d\n", -errno);
          close(fd);
          return EXIT_FAILURE;
        }

      if (nread != (ssize_t)sample_size)
        {
          printf("usensor_reader short read nread=%zd expected=%zu\n", nread, sample_size);
          close(fd);
          return EXIT_FAILURE;
        }

      print_sensor_sample(kind, path, &sample);
    }

  close(fd);
  return EXIT_SUCCESS;
}
