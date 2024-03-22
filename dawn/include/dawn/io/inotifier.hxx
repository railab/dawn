// dawn/include/dawn/io/inotifier.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/porting/config.hxx"

namespace dawn
{
// Forward declaration

class CIOCommon;
struct io_ddata_t;

/**
 * @brief Abstract interface for registering asynchronous I/O notification.
 *
 * Enables event-driven architecture where I/O data arrival triggers callback
 * execution from a dedicated worker thread.
 */

class IIONotifier
{
public:
  /**
   * @brief Notifier callback function type.
   *
   * Callback signature for I/O notification events.
   *
   * @param priv Private context pointer registered with callback.
   * @param data Dynamically allocated I/O data (io_ddata_t*).
   * @return Reserved for future use (currently unused).
   */

  typedef int (*notifier_cb_t)(void *priv, io_ddata_t *data);

  /**
   * @brief Notifier registration structure.
   *
   * Configuration for a single I/O notification callback.
   */

  struct
  {
    /**
     * @brief Private context pointer.
     *
     * User-defined context passed to callback function.
     */

    void *priv;

    /**
     * @brief I/O object pointer.
     *
     * Pointer to CIOCommon object that will be monitored for data
     * availability.
     */

    CIOCommon *io;

    /**
     * @brief Notifier callback function type.
     *
     * Callback signature for I/O notification events.
     */

    int (*cb)(void *priv, io_ddata_t *data);

    /**
     * @brief Notification callback priority.
     *
     * Priority level for callback execution.
     */

    int prio;
  } typedef SIONotifier;

  /**
   * @brief Register I/O notification callback.
   *
   * Implements IIONotifier interface.
   *
   * @param n SIONotifier structure containing callback configuration.
   * @return OK on success, negative error code on failure.
   */

  virtual int regNotifier(SIONotifier n) = 0;

  /**
   * @brief Emit an immediate notification for already-available data.
   *
   * Poll-based notifiers still use regNotifier() for fd-driven updates; this
   * hook covers synchronous writes where the payload is already available.
   *
   * @param io I/O object that produced the data.
   * @param data Data payload to pass to callbacks.
   * @return OK on success, negative error code on failure.
   */

  virtual int notifyData(CIOCommon *io, io_ddata_t *data)
  {
    (void)io;
    (void)data;
    return -ENOTSUP;
  }
};

} // Namespace dawn
