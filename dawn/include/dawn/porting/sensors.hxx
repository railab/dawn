// dawn/include/dawn/porting/sensors.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/nuttx/sensors.hxx"

/**
 * @brief Open sensor device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int sensor_open(const char *path);

/**
 * @brief Set the sensor output/measurement interval.
 *
 * @param fd File descriptor.
 * @param interval_us Interval between samples in microseconds. For GNSS this
 *                    maps to the fix interval (0 = single fix, otherwise the
 *                    periodic/continuous fix rate).
 * @return Zero on success or a negative error code.
 */

int sensor_set_interval(int fd, uint32_t interval_us);

/**
 * @brief Close sensor device.
 *
 * @param fd File descriptor.
 */

void sensor_close(int fd);

/**
 * @brief Read sensor data.
 *
 * @param fd File descriptor.
 * @param data Pointer to buffer to receive data.
 * @param len Length of buffer in bytes.
 * @return Number of bytes read or negative error code.
 */

int sensor_read(int fd, void *data, size_t len);

/**
 * @brief Register a userspace-provided sensor topic.
 *
 * @param path Full sensor device path.
 * @param event_size Size of one NuttX sensor event struct.
 * @param queue_size Number of events in the sensor circular buffer.
 * @param persist Whether the latest value remains readable by late readers.
 * @return OK, -EEXIST if already present, or negative error code.
 */

int sensor_user_register(const char *path, size_t event_size, uint32_t queue_size, bool persist);

/**
 * @brief Unregister a userspace-provided sensor topic.
 *
 * @param path Full sensor device path.
 * @return OK or negative error code.
 */

int sensor_user_unregister(const char *path);

/**
 * @brief Open sensor device for publishing.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int sensor_open_write(const char *path);

/**
 * @brief Write one or more sensor event structs.
 *
 * @param fd Sensor file descriptor.
 * @param data Pointer to event data.
 * @param len Data length in bytes.
 * @return Number of bytes written or negative error code.
 */

int sensor_write(int fd, const void *data, size_t len);
