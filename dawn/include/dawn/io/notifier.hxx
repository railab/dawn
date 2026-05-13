// dawn/include/dawn/io/notifier.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <poll.h>

#include <atomic>
#include <mutex>

#include "dawn/common/thread.hxx"
#include "dawn/io/factory.hxx"
#include "dawn/io/inotifier.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
// Forward declaration

struct io_ddata_t;

/**
 * @brief I/O notification handler with poll-based event delivery.
 *
 * Implements asynchronous I/O notification using POSIX poll() system call.
 */

class CIONotifier final : public IIONotifier
{
public:
  CIONotifier()
    : pfds(nullptr)
    , pfdsLen(0)
    , pfdsUpdate(false)
    , threadCtl()
  {
  }

  ~CIONotifier();

  void setThreadConfig(const CThreadedObject::SThreadConfig &config)
  {
    threadCtl.setThreadConfig(config);
  }

  void setThreadStackSize(size_t stackSize)
  {
    threadCtl.setThreadStackSize(stackSize);
  }

  void setThreadPriority(int priority)
  {
    threadCtl.setThreadPriority(priority);
  }

  void setThreadScheduler(int scheduler)
  {
    threadCtl.setThreadScheduler(scheduler);
  }

  int regNotifier(SIONotifier n) override;
  int notifyData(CIOCommon *io, io_ddata_t *data) override;
  int start();
  int stop();

private:
  struct
  {
    std::vector<SIONotifier> user; ///< Registered user callbacks for this I/O.
    io_ddata_t *data; ///< Dynamically allocated data for this I/O (freed by last callback).
    size_t batch;     ///< Samples per poll event (from IO_CFG_NOTIFY).
  } typedef SIONotifierPriv;

  std::vector<SIONotifierPriv> vnote; ///< Registered notifications, one per I/O.
  struct pollfd *pfds;                ///< POSIX poll fd array.
  size_t pfdsLen;                     ///< Poll array length.
  std::atomic_bool pfdsUpdate;        ///< Set when poll array needs rebuild.
  std::mutex pfdsLock;                ///< Lock protecting pfds during updates.

  CThreadedObject threadCtl;          ///< Worker thread lifecycle controller.

  void updatePfds();
  void thread();
};
} // Namespace dawn
