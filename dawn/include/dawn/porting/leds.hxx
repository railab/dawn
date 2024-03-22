// dawn/include/dawn/porting/leds.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include "dawn/porting/nuttx/leds.hxx"

/**
 * @brief Open LED device.
 *
 * @param path Path to LED device node.
 * @return File descriptor on success, or negative error code on failure.
 */

int leds_open(const char *path);

/**
 * @brief Close LED device.
 *
 * @param fd File descriptor returned by leds_open().
 */

void leds_close(int fd);

/**
 * @brief Read current LED bitmask from LED device.
 *
 * @param fd File descriptor returned by leds_open().
 * @param leds Output pointer receiving current LED bitmask.
 * @return OK on success, or negative error code on failure.
 */

int leds_read(int fd, uint32_t *leds);

/**
 * @brief Write LED bitmask to LED device.
 *
 * @param fd File descriptor returned by leds_open().
 * @param leds LED bitmask value to set.
 * @return OK on success, or negative error code on failure.
 */

int leds_write(int fd, uint32_t leds);
