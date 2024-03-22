// dawn/include/dawn/io/notify_manager.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/io/common.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/porting/config.hxx"
#ifdef CONFIG_DAWN_IO_NOTIFY_STREAM
#  include "dawn/io/stream_notifier.hxx"
#endif

namespace dawn
{
/**
 * @brief Manages notifier instances for I/O objects.
 *
 * Routes I/O objects to the appropriate notifier instance based on their
 * configured notifier type and priority. Creates notifier instances on
 * demand during initialization.
 *
 * For poll-based notifiers, I/O objects with the same priority share
 * a single notifier instance (and thread). Different priorities get
 * separate instances.
 */

class CIONotifierManager
{
public:
  CIONotifierManager();
  ~CIONotifierManager();

  int regIO(CIOCommon *io);
  int start();
  int stop();

private:
  struct SNotifierEntry
  {
    uint8_t type; ///< Notifier type (IO_NOTIFY_POLL, etc.)
    int prio;     ///< Thread priority

    union
    {
      CIONotifier *poll;       ///< Poll notifier instance
#ifdef CONFIG_DAWN_IO_NOTIFY_STREAM
      CStreamNotifier *stream; ///< Stream notifier instance
#endif
    };
  };

  std::vector<SNotifierEntry> entries; ///< Collection of notifier instances.

  CIONotifier *findPoll(int prio);
};

} // Namespace dawn
