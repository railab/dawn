// dawn/include/dawn/io/limits.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <cstdint>

#include "dawn/common/object.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Common IO runtime limits container and validator.
 *
 * Stores descriptor-backed min/max/step arrays and validates payload words
 * against configured limits.
 */

class CIOLimits
{
public:
  /**
   * @brief Common IO limit configuration identifiers. Must match IO_CFG_*.
   */

  enum
  {
    CFG_LIMIT_MIN = 2, ///< Minimum limit words
    CFG_LIMIT_MAX = 3, ///< Maximum limit words
    CFG_LIMIT_STEP = 4 ///< Step limit words
  };

  /**
   * @brief Construct empty limits.
   */

  CIOLimits()
  {
    reset();
  }

  /**
   * @brief Reset all configured limits.
   */

  void reset()
  {
#ifdef CONFIG_DAWN_IO_LIMITS
    minData = nullptr;
    maxData = nullptr;
    stepData = nullptr;
    nWords = 0;
    cfgDtype = SObjectId::DTYPE_ANY;
#endif
  }

  /**
   * @brief Bind one limit config item.
   *
   * @param id Limit config ID (CFG_LIMIT_MIN/MAX/STEP).
   * @param dtype Config data type.
   * @param words Number of uint32 words in limit array.
   * @param data Pointer to descriptor-backed limit words.
   * @return OK on success, negative error code on failure.
   */

  int bind(uint8_t id, uint8_t dtype, size_t words, const uint32_t *data);

  /**
   * @brief Check whether any limits are configured.
   */

  bool isConfigured() const
  {
#ifdef CONFIG_DAWN_IO_LIMITS
    return nWords > 0;
#else
    return false;
#endif
  }

  /**
   * @brief Validate payload against configured limits.
   *
   * @param data Payload words.
   * @param words Payload size in uint32 words.
   * @param dtype Payload data type.
   * @return OK on success, negative error code on failure.
   */

  int validate(const uint32_t *data, size_t words, uint8_t dtype) const;

  /**
   * @brief Get minimum limit words pointer.
   */

  const uint32_t *getMin() const
  {
#ifdef CONFIG_DAWN_IO_LIMITS
    return minData;
#else
    return nullptr;
#endif
  }

  /**
   * @brief Get maximum limit words pointer.
   */

  const uint32_t *getMax() const
  {
#ifdef CONFIG_DAWN_IO_LIMITS
    return maxData;
#else
    return nullptr;
#endif
  }

  /**
   * @brief Get step limit words pointer.
   */

  const uint32_t *getStep() const
  {
#ifdef CONFIG_DAWN_IO_LIMITS
    return stepData;
#else
    return nullptr;
#endif
  }

  /**
   * @brief Get configured limit array size in words.
   */

  size_t getWords() const
  {
#ifdef CONFIG_DAWN_IO_LIMITS
    return nWords;
#else
    return 0;
#endif
  }

private:
#ifdef CONFIG_DAWN_IO_LIMITS
  const uint32_t *minData;
  const uint32_t *maxData;
  const uint32_t *stepData;
  size_t nWords;
  uint8_t cfgDtype;
#endif
};

#ifndef CONFIG_DAWN_IO_LIMITS
/**
 * @brief No-op bind when IO limits feature is disabled.
 */

inline int CIOLimits::bind(uint8_t id, uint8_t dtype, size_t words, const uint32_t *data)
{
  (void)id;
  (void)dtype;
  (void)words;
  (void)data;
  return OK;
}

/**
 * @brief No-op validation when IO limits feature is disabled.
 */

inline int CIOLimits::validate(const uint32_t *data, size_t words, uint8_t dtype) const
{
  (void)data;
  (void)words;
  (void)dtype;
  return OK;
}
#endif
} // Namespace dawn
