// dawn/include/dawn/prog/common.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdlib>

#include "dawn/common/bindable.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;

/**
 * @brief Base class for all PROG (processing) objects.
 *
 * Provides common interface for data processing programs (Sampling, Adjust,
 * Stats).
 */

class CProgCommon : public CBindableObject
{
public:
  /** @brief Program object class types. */

  enum
  {
    /** @brief Generic PROG type. */

    PROG_CLASS_ANY = 0,

    /** @brief Minimum value tracker. */

    PROG_CLASS_STATS_MIN = 1,

    /** @brief Maximum value tracker. */

    PROG_CLASS_STATS_MAX = 2,

    /** @brief Running average calculator. */

    PROG_CLASS_STATS_AVG = 3,

    /** @brief Sum accumulator. */

    PROG_CLASS_STATS_SUM = 4,

    /** @brief Sample counter. */

    PROG_CLASS_STATS_COUNT = 5,

    /** @brief Running RMS (root mean square) calculator. */

    PROG_CLASS_STATS_RMS = 8,

    /** @brief Periodic data sampler. */

    PROG_CLASS_SAMPLING = 6,

    /** @brief Dummy program (test/helper). */

    PROG_CLASS_DUMMY = 7,

    /** @brief Scale/offset adjustment. */

    PROG_CLASS_ADJUST = 10,

    /** @brief Protocol-to-protocol IO gateway. */

    PROG_CLASS_GATEWAY = 11,

    /** @brief Cache latest notified sample for fetch-based readers. */

    PROG_CLASS_LATEST = 12,

    /** @brief Generic input-to-output routing bridge. */

    PROG_CLASS_REDIRECT = 13,

    /** @brief Moving average filter. */

    PROG_CLASS_MOVING_AVG = 14,

    /** @brief First-order IIR filter. */

    PROG_CLASS_IIR_FILTER = 15,

    /** @brief Threshold and hysteresis comparator. */

    PROG_CLASS_THRESHOLD = 16,

    /** @brief Threshold comparator returning gated source value. */

    PROG_CLASS_THRESHOLD_VALUE = 17,

    /** @brief Notify-driven history capture buffer. */

    PROG_CLASS_BUFFER = 18,

    /** @brief Periodic state sequencer. */

    PROG_CLASS_SEQUENCER = 19,

    PROG_CLASS_BITSPLIT = 20,
    PROG_CLASS_TOGGLE = 21,
    PROG_CLASS_COUNTER = 22,
    PROG_CLASS_SWITCH = 23,
    PROG_CLASS_EXPRESSION = 24,
    PROG_CLASS_SELECTOR = 25,
    PROG_CLASS_BITPACK = 26,
    PROG_CLASS_CONFIGWRITER = 27,
    PROG_CLASS_VECPACK = 28,
    PROG_CLASS_VECSPLIT = 29,
    PROG_CLASS_MANYTOONE = 30,
    PROG_CLASS_ONETOMANY = 31,
    PROG_CLASS_IOMUX = 32,
    PROG_CLASS_IODEMUX = 33,

    /** @brief User-defined PROG types start here. */

    PROG_CLASS_USER = 511,
    PROG_CLASS_LAST
  } typedef EProgClass;

  static_assert(PROG_CLASS_LAST - 1 <= SObjectId::CLS_MAX);

  /** @brief Common configuration IDs. */

  enum
  {
    PROG_CFG_FIRST = 0,
    PROG_CFG_LAST = 31
  };

  /**
   * @brief Constructor.
   *
   * Initializes base PROG object with metadata from descriptor.
   *
   * @param[in] desc Program descriptor with configuration.
   */

  explicit CProgCommon(CDescObject &desc);

protected:
  int prepareWritableTarget(CIOCommon *io, size_t dim, bool notify);
};
} // Namespace dawn
