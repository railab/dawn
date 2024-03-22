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
