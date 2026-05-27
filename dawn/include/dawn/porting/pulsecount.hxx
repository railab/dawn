// dawn/include/dawn/porting/pulsecount.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include "dawn/porting/nuttx/pulsecount.hxx"

namespace dawn
{
namespace porting
{
/**
 * @brief Pulse-count output settings.
 * @ingroup porting
 */

struct pulsecount_write_s
{
  uint32_t high_ns;
  uint32_t low_ns;
  uint32_t count;
};

} // namespace porting
} // namespace dawn

/**
 * @brief Open pulsecount device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int pulsecount_open(const char *path);

/**
 * @brief Close pulsecount device.
 *
 * @param fd File descriptor.
 */

void pulsecount_close(int fd);

/**
 * @brief Initialize pulsecount device.
 *
 * @param fd File descriptor.
 */

void pulsecount_init(int fd);

/**
 * @brief Start the configured pulse train.
 *
 * @param fd File descriptor.
 * @return OK on success, negative error code on failure.
 */

int pulsecount_start(int fd);

/**
 * @brief Stop pulse generation.
 *
 * @param fd File descriptor.
 * @return OK on success, negative error code on failure.
 */

int pulsecount_stop(int fd);

/**
 * @brief Write pulse train settings.
 *
 * @param fd File descriptor.
 * @param pulsecount Pointer to pulsecount_write_s structure.
 * @return OK on success, negative error code on failure.
 */

int pulsecount_write(int fd, const dawn::porting::pulsecount_write_s *pulsecount);
