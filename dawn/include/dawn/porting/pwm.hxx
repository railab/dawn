// dawn/include/dawn/porting/pwm.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

#include "dawn/porting/nuttx/pwm.hxx"

namespace dawn
{
namespace porting
{
/**
 * @brief PWM write data.
 * @ingroup porting
 */

struct pwm_write_s
{
  uint32_t freq;

  struct
  {
    uint16_t duty;   // Duty in 0.1% units (0 - 0%, 10 - 1%, 1000 - 100%)
    uint8_t channel; // start from 1
  } channels[];
};

} // namespace porting
} // namespace dawn

/**
 * @brief Open PWM device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int pwm_open(const char *path);

/**
 * @brief Close PWM device.
 *
 * @param fd File descriptor.
 */

void pwm_close(int fd);

/**
 * @brief Initialize PWM device.
 *
 * @param fd File descriptor.
 */

void pwm_init(int fd);

/**
 * @brief Start PWM device.
 *
 * @param fd File descriptor.
 */

int pwm_start(int fd);

/**
 * @brief Stop PWM device.
 *
 * @param fd File descriptor.
 */

int pwm_stop(int fd);

/**
 * @brief Write PWM data.
 *
 * @param fd File descriptor.
 * @param pwm Pointer to pwm_write_s structure.
 * @return OK on success, negative error code on failure.
 */

int pwm_write(int fd, dawn::porting::pwm_write_s *pwm);
