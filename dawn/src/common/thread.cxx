// dawn/src/common/thread.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/thread.hxx"

#include <new>

#include "dawn/debug.hxx"

using namespace dawn;

CThreadedObject::~CThreadedObject()
{
  if (th != nullptr)
    {
      if (th->joinable())
        {
          thQuit = true;
          th->join();
        }

      delete th;
      th = nullptr;
    }

  thQuitDone = true;
}

int CThreadedObject::threadStart()
{
  if (th != nullptr)
    {
      if (!thQuitDone)
        {
          DAWNERR("Thread already running\n");
          return -EBUSY;
        }

      if (th->joinable())
        {
          th->join();
        }

      delete th;
      th = nullptr;
    }

  // Verify that thread function has been set

  if (!threadFunc)
    {
      DAWNERR("Thread function not set before calling threadStart()\n");
      return -EINVAL;
    }

  // Initialize quit flags

  thQuit = false;
  thQuitDone = false;

  // Create and start worker thread

  th = new (std::nothrow) std::thread([this]() { this->threadWrapper(); });
  if (!th)
    {
      DAWNERR("Failed to create worker thread\n");
      thQuit = true;
      thQuitDone = true;
      return -ENOMEM;
    }

  return 0;
}

int CThreadedObject::threadStop()
{
  // Signal thread to quit

  thQuit = true;

  if (th != nullptr)
    {
      if (th->joinable())
        {
          th->join();
        }

      delete th;
      th = nullptr;
    }

  thQuitDone = true;

  return 0;
}

bool CThreadedObject::isRunning() const
{
  return !thQuitDone;
}

void CThreadedObject::threadWrapper()
{
  // Call the registered thread function

  if (threadFunc)
    {
      threadFunc();
    }

  // Mark as complete when thread routine finishes

  thQuitDone = true;
}
