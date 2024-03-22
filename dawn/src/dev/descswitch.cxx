// dawn/src/dev/descswitch.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/dev/descswitch.hxx"

#include <atomic>

using namespace dawn;

namespace
{
std::atomic_bool g_switchRequested(false);
std::atomic_uint8_t g_switchSlot(0);
std::atomic_uint8_t g_activeSlot(0);
}

void CDescSwitch::requestSwitch(uint8_t slot)
{
  g_switchSlot = slot;
  g_switchRequested = true;
}

void CDescSwitch::clear()
{
  g_switchRequested = false;
}

bool CDescSwitch::isSwitchRequested()
{
  return g_switchRequested.load();
}

uint8_t CDescSwitch::getSwitchSlot()
{
  return g_switchSlot.load();
}

uint8_t CDescSwitch::getActiveSlot()
{
  return g_activeSlot.load();
}

void CDescSwitch::setActiveSlot(uint8_t slot)
{
  g_activeSlot = slot;
}
