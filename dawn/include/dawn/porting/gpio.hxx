// dawn/include/dawn/porting/gpio.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/nuttx/gpio.hxx"

/**
 * @brief Open GPIO input device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int gpi_open(const char *path);

/**
 * @brief Close GPIO input device.
 *
 * @param fd File descriptor.
 */

void gpi_close(int fd);

/**
 * @brief Initialize GPIO input device.
 *
 * @param fd File descriptor.
 * @return OK on success, negative error code on failure.
 */

int gpi_init(int fd);

/**
 * @brief Read GPIO input value.
 *
 * @param fd File descriptor.
 * @param invalue Pointer to store boolean value.
 * @return OK on success, negative error code on failure.
 */

int gpi_read(int fd, bool *invalue);

/**
 * @brief Enable/check GPIO notification.
 *
 * @param fd File descriptor.
 * @return True if notification is supported/enabled.
 */

bool gpi_notify(int fd);

/**
 * @brief Open GPIO output device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int gpo_open(const char *path);

/**
 * @brief Close GPIO output device.
 *
 * @param fd File descriptor.
 */

void gpo_close(int fd);

/**
 * @brief Initialize GPIO output device.
 *
 * @param fd File descriptor.
 * @return OK on success, negative error code on failure.
 */

int gpo_init(int fd);

/**
 * @brief Read GPIO output value (current state).
 *
 * @param fd File descriptor.
 * @param invalue Pointer to store boolean value.
 * @return OK on success, negative error code on failure.
 */

int gpo_read(int fd, bool *invalue);

/**
 * @brief Write GPIO output value.
 *
 * @param fd File descriptor.
 * @param invalue Boolean value to write.
 * @return OK on success, negative error code on failure.
 */

int gpo_write(int fd, bool invalue);
