// dawn/src/proto/nimble/hci.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/hci.hxx"

using namespace dawn;

extern "C"
{
  void ble_hci_sock_ack_handler(void *param);
};

void CProtoNimbleHci::thread()
{
  DAWNINFO("start HCI thread\n");

  // Handle NimBLE socket task

#ifdef CONFIG_DAWN_PROTO_NIMBLE_DUMMY
  do
    {
      sleep(1);
    }
  while (!threadCtl.shouldQuit());
#else
  ble_hci_sock_ack_handler(nullptr);
#endif

  // Clean thread

  DAWNINFO("clean HCI thread\n");
}

void CProtoNimbleHci::start()
{
  threadCtl.setThreadFunc([this]() { thread(); });
  threadCtl.threadStart();
}

void CProtoNimbleHci::stop()
{
  // Stop hci

#ifdef CONFIG_DAWN_PROTO_NIMBLE_DUMMY
  threadCtl.requestStop();
#else
  // Upstream NimBLE has no clean HCI shutdown API; clean stop is only
  // supported with CONFIG_DAWN_PROTO_NIMBLE_DUMMY.

  DAWNERR("nimble HCI stop not supported on real backend\n");
#endif

  // Wait for thread exit

  threadCtl.threadStop();
}

bool CProtoNimbleHci::isRunning() const
{
  return threadCtl.isRunning();
}
