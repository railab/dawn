// dawn/include/dawn/porting/adc.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>
#include <cstdio>

#include "dawn/porting/nuttx/adc.hxx"

namespace dawn
{
namespace porting
{
/**
 * @brief ADC read data.
 * @ingroup porting
 */

struct adc_read_s
{
  int32_t data; // Do not read channel !
};

} // namespace porting
} // namespace dawn

/**
 * @brief Open ADC device.
 *
 * @param path Device path.
 * @return File descriptor or negative error code.
 */

int adc_open(const char *path);

/**
 * @brief Close ADC device.
 *
 * @param fd File descriptor.
 */

void adc_close(int fd);

/**
 * @brief Get number of ADC channels.
 *
 * @param fd File descriptor.
 * @return Number of channels or negative error code.
 */

int adc_get_nchans(int fd);

/**
 * @brief Start ADC conversion.
 *
 * @param fd File descriptor.
 * @return OK on success, negative error code on failure.
 */

int adc_start(int fd);

/**
 * @brief Stop ADC conversion.
 *
 * @param fd File descriptor.
 * @return OK on success, negative error code on failure.
 */

int adc_stop(int fd);

/**
 * @brief Adjust ADC trigger timer frequency.
 *
 * @param fd File descriptor.
 * @param freq_hz Requested trigger frequency in Hz.
 * @return OK on success, negative error code on failure.
 */

int adc_set_timer_freq(int fd, uint32_t freq_hz);

/**
 * @brief Query the number of pending ADC samples.
 *
 * @param fd File descriptor.
 * @return Pending sample count or negative error code.
 */

int adc_get_samples_count(int fd);

/**
 * @brief Read ADC data.
 *
 * @param fd File descriptor.
 * @param adc Pointer to array of adc_read_s structures.
 * @param len Number of samples to read.
 * @return Number of samples read or negative error code.
 */

int adc_read(int fd, dawn::porting::adc_read_s *adc, size_t len);
