// dawn/include/dawn/porting/rgbled.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>

#include "dawn/porting/nuttx/rgbled.hxx"

/**
 * @brief Open RGB LED device.
 *
 * @param path Path to RGB LED device node.
 * @return File descriptor on success, or negative error code on failure.
 */

int rgbled_open(const char *path);

/**
 * @brief Close RGB LED device.
 *
 * @param fd File descriptor returned by rgbled_open().
 */

void rgbled_close(int fd);

/**
 * @brief Write RGB LED color string to a device.
 *
 * @param fd File descriptor returned by rgbled_open().
 * @param rgb Input #RRGGBB string.
 * @param len Number of bytes to write.
 * @return OK on success, or negative error code on failure.
 */

int rgbled_write(int fd, const char *rgb, size_t len);
