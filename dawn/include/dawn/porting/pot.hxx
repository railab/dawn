// dawn/include/dawn/porting/pot.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include "dawn/porting/nuttx/pot.hxx"

namespace dawn
{
namespace porting
{
/**
 * @brief POT wiper access data.
 * @ingroup porting
 */

struct pot_rw_s
{
  uint8_t wiper; // wiper index
  uint32_t val;  // wiper value
};

} // namespace porting

} // namespace dawn

/**
 * @brief Open POT device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int pot_open(const char *path);

/**
 * @brief Close POT device.
 *
 * @param fd File descriptor.
 */

void pot_close(int fd);

/**
 * @brief Get the POT wiper full scale value.
 *
 * @param fd File descriptor.
 * @param max Pointer to the returned full scale value.
 * @return OK on success, negative error code on failure.
 */

int pot_get_max(int fd, uint32_t *max);

/**
 * @brief Set a POT wiper value.
 *
 * @param fd File descriptor.
 * @param pot Pointer to dawn::porting::pot_rw_s structure.
 * @return OK on success, negative error code on failure.
 */

int pot_set_wiper(int fd, dawn::porting::pot_rw_s *pot);

/**
 * @brief Get a POT wiper value.
 *
 * @param fd File descriptor.
 * @param pot Pointer to dawn::porting::pot_rw_s structure.
 * @return OK on success, negative error code on failure.
 */

int pot_get_wiper(int fd, dawn::porting::pot_rw_s *pot);
