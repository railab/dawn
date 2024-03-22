// dawn/include/dawn/porting/encoder.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include "dawn/porting/nuttx/encoder.hxx"

namespace dawn
{
namespace porting
{
/**
 * @brief Quadrature encoder index sample.
 * @ingroup porting
 */

struct encoder_index_s
{
  int32_t qenc_pos;
  int32_t indx_pos;
  int16_t indx_cnt;
};

} // namespace porting
} // namespace dawn

/**
 * @brief Open encoder device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int encoder_open(const char *path);

/**
 * @brief Close encoder device.
 *
 * @param fd File descriptor.
 */

void encoder_close(int fd);

/**
 * @brief Read encoder position
 *
 * @param fd File descriptor.
 * @param pos pointer to int32_t position.
 */

int encoder_read_position(int fd, int32_t *pos);

/**
 * @brief Read encoder postion with index
 *
 * @param fd File descriptor.
 * @param index pointer to index position.
 */

int encoder_read_index(int fd, dawn::porting::encoder_index_s *index);

/**
 * @brief Reset encoder position
 *
 * @param fd File descriptor.
 */

int encoder_reset(int fd);

/**
 * @brief Set maximum encoder position
 *
 * @param fd File descriptor.
 * @param posmax maximum encoder position
 */

int encoder_set_posmax(int fd, uint32_t posmax);
