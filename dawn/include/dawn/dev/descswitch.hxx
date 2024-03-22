// dawn/include/dawn/dev/descswitch.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstdint>

namespace dawn
{
/**
 * @brief Global descriptor switch request manager.
 *
 * Holds pending switch request and active slot state for runtime descriptor
 * reload flow.
 */

class CDescSwitch
{
public:
  /**
   * @brief Request descriptor switch to a slot.
   *
   * @param slot Target descriptor slot index.
   */

  static void requestSwitch(uint8_t slot);

  /** @brief Clear pending switch request. */

  static void clear();

  /**
   * @brief Check if descriptor switch is pending.
   *
   * @return True when a switch was requested.
   */

  static bool isSwitchRequested();

  /**
   * @brief Get pending switch target slot.
   *
   * @return Target slot index.
   */

  static uint8_t getSwitchSlot();

  /**
   * @brief Get currently active descriptor slot.
   *
   * @return Active slot index.
   */

  static uint8_t getActiveSlot();

  /**
   * @brief Set currently active descriptor slot.
   *
   * @param slot Active slot index.
   */

  static void setActiveSlot(uint8_t slot);
};

} // namespace dawn
