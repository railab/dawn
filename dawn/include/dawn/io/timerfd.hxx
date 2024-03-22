// dawn/include/dawn/io/timerfd.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <mutex>

#include "dawn/io/timerfd.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Internal helper for timer-based I/O notifications.
 *
 * Provides a base class for I/O types that require periodic notifications
 */

class CIOTimerfd
{
public:
#ifdef CONFIG_DAWN_IO_NOTIFY
  int fd;            ///< File descriptor for the timerfd.
  uint32_t interval; ///< Current notification interval in microseconds.
#endif

  CIOTimerfd()
#ifdef CONFIG_DAWN_IO_NOTIFY
    : fd(-1)
    , interval(0)
#endif
  {
  }

  ~CIOTimerfd();

  int timfd_init();
  void timfd_interval(uint32_t data);
  void timfd_ack();
  int timfd_fd() const;
  int timfd_start();
  int timfd_stop();
};
} // Namespace dawn
