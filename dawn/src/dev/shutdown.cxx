// dawn/src/dev/shutdown.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/dev/shutdown.hxx"

#include <atomic>

using namespace dawn;

namespace
{
std::atomic_bool g_shutdownRequested(false);
}

void CShutdown::request()
{
  g_shutdownRequested = true;
}

void CShutdown::clear()
{
  g_shutdownRequested = false;
}

bool CShutdown::isRequested()
{
  return g_shutdownRequested.load();
}
