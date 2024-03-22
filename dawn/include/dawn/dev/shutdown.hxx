// dawn/include/dawn/dev/shutdown.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

namespace dawn
{
/**
 * @brief Global shutdown request manager.
 *
 * Provides a thread-safe way to request and check for application-wide
 * shutdown signals.
 */

class CShutdown
{
public:
  /** @brief Request application shutdown */

  static void request();

  /** @brief Clear pending shutdown request */

  static void clear();

  /**
   * @brief Check if shutdown has been requested.
   *
   * @return True if shutdown is requested, false otherwise.
   */

  static bool isRequested();
};
} // namespace dawn
