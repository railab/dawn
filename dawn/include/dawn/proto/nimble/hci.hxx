// dawn/include/dawn/proto/nimble/hci.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include "dawn/common/thread.hxx"
#include "dawn/debug.hxx"
#include "dawn/porting/config.hxx"
#include "host/ble_gap.h"

namespace dawn
{
/**
 * @brief Host Controller Interface (HCI) management for NimBLE.
 *
 * This class manages the HCI layer which provides communication between the
 * BLE host and the BLE controller hardware.
 */

class CProtoNimbleHci
{
public:
  CProtoNimbleHci()
    : threadCtl()
  {
  }

  ~CProtoNimbleHci() = default;

  void start();
  void stop();
  bool isRunning() const;

private:
  CThreadedObject threadCtl; ///< Thread management for HCI event processing.

  void thread();
};
} // Namespace dawn
