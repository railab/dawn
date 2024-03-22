// dawn/include/dawn/porting/dac.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include "dawn/porting/nuttx/dac.hxx"

namespace dawn
{
namespace porting
{
/**
 * @brief DAC write data.
 * @ingroup porting
 */

struct dac_write_s
{
  int32_t data; // channel not used
};

} // namespace porting

} // namespace dawn

/**
 * @brief Open DAC device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int dac_open(const char *path);

/**
 * @brief Close DAC device.
 *
 * @param fd File descriptor.
 */

void dac_close(int fd);

/**
 * @brief Initialize DAC device.
 *
 * @param fd File descriptor.
 */

void dac_init(int fd);

/**
 * @brief Write DAC data.
 *
 * @param fd File descriptor.
 * @param dac Pointer to dawn::porting::dac_write_s structure.
 * @return OK on success, negative error code on failure.
 */

int dac_write(int fd, dawn::porting::dac_write_s *dac);
