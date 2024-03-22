// dawn/src/proto/nimble/host.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nimble/host.hxx"

#include "modbus/mb.h"
#include "modbus/mbport.h"
#include "nimble/nimble_port.h"

using namespace dawn;

void CProtoNimbleHost::thread()
{
  DAWNINFO("start Host thread\n");

  // Handle NimBLE Host task

#ifdef CONFIG_DAWN_PROTO_NIMBLE_DUMMY
  do
    {
      sleep(1);
    }
  while (!threadCtl.shouldQuit());
#else
  nimble_port_run();
#endif

  // Clean thread

  DAWNINFO("clean Host thread\n");
}

void CProtoNimbleHost::start()
{
  threadCtl.setThreadFunc([this]() { thread(); });
  threadCtl.threadStart();
}

void CProtoNimbleHost::stop()
{
  // Stop host

#ifdef CONFIG_DAWN_PROTO_NIMBLE_DUMMY
  threadCtl.requestStop();
#else
  // Upstream NimBLE has no clean host shutdown API; clean stop is only
  // supported with CONFIG_DAWN_PROTO_NIMBLE_DUMMY.

  DAWNERR("nimble host stop not supported on real backend\n");
#endif

  // Wait for thread exit

  threadCtl.threadStop();
}

bool CProtoNimbleHost::isRunning() const
{
  return threadCtl.isRunning();
}
