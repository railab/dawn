// dawn/include/dawn/proto/nimble/host.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/thread.hxx"
#include "dawn/porting/config.hxx"
#include "dawn/proto/common.hxx"
#include "host/ble_gap.h"

namespace dawn
{
/**
 * @brief Manages NimBLE Host controller initialization and event processing.
 *
 * This class is responsible for initializing and managing the Apache NimBLE
 * host stack on embedded systems.
 */

class CProtoNimbleHost
{
public:
  CProtoNimbleHost()
    : threadCtl()
  {
  }

  ~CProtoNimbleHost() = default;

  void start();
  void stop();
  bool isRunning() const;

private:
  CThreadedObject threadCtl; ///< Thread management for host event loop.

  void thread();
};
} // Namespace dawn
