// dawn/include/dawn/io/stream_notifier.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <vector>

#include "dawn/common/thread.hxx"
#include "dawn/io/inotifier.hxx"
#include "dawn/porting/config.hxx"

namespace dawn
{
// Forward declaration

struct io_ddata_t;

/**
 * @brief Stream-based I/O notifier with dedicated thread per I/O.
 *
 * Provides a dedicated worker thread for a single I/O object, suitable for
 * high-frequency data streaming where each I/O needs its own thread and
 * priority level. Uses poll() on a single file descriptor with timeout
 * for clean thread shutdown.
 *
 * Unlike CIONotifier which multiplexes multiple I/Os per thread via poll(),
 * CStreamNotifier assigns one thread per I/O for minimal contention and
 * independent priority scheduling.
 */

class CStreamNotifier final : public IIONotifier
{
public:
  CStreamNotifier();
  ~CStreamNotifier();

  CStreamNotifier(const CStreamNotifier &) = delete;
  CStreamNotifier &operator=(const CStreamNotifier &) = delete;

  int regNotifier(SIONotifier n) override;
  int start();
  int stop();

private:
  std::vector<SIONotifier> users; ///< Registered user callbacks for the bound I/O.
  CIOCommon *io;                  ///< Bound I/O (one per stream notifier).
  io_ddata_t *data;               ///< Data buffer for the bound I/O.
  size_t batch;                   ///< Samples per poll event (from IO_CFG_NOTIFY).
  CThreadedObject threadCtl;      ///< Worker thread lifecycle controller.

  void thread();
};
} // Namespace dawn
