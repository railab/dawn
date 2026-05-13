// dawn/src/common/thread.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/common/thread.hxx"

#include <cerrno>
#include <cstring>
#include <sched.h>

#include "dawn/debug.hxx"

using namespace dawn;

CThreadedObject::~CThreadedObject()
{
  if (thCreated)
    {
      thQuit = true;
      (void)joinThread();
    }

  thQuitDone = true;
}

int CThreadedObject::joinThread()
{
  int ret;

  if (!thCreated)
    {
      return OK;
    }

  ret = pthread_join(th, nullptr);
  if (ret != 0)
    {
      DAWNERR("Failed to join worker thread (%d)\n", ret);
      return -ret;
    }

  thCreated = false;
  std::memset(&th, 0, sizeof(th));
  return OK;
}

int CThreadedObject::buildThreadAttr(pthread_attr_t &attr, bool &needsDestroy) const
{
  struct sched_param param;
  int maxPrio;
  int minPrio;
  int policy;
  int priority;
  int ret;

  needsDestroy = false;

  if (threadConfig.priority < THREAD_PRIORITY_DEFAULT)
    {
      DAWNERR("Invalid worker thread priority %d\n", threadConfig.priority);
      return -EINVAL;
    }

  if (threadConfig.scheduler < THREAD_SCHEDULER_DEFAULT)
    {
      DAWNERR("Invalid worker thread scheduler %d\n", threadConfig.scheduler);
      return -EINVAL;
    }

  ret = pthread_attr_init(&attr);
  if (ret != 0)
    {
      DAWNERR("Failed to initialize worker thread attributes (%d)\n", ret);
      return -ret;
    }

  needsDestroy = true;

  if (threadConfig.stackSize > 0)
    {
      ret = pthread_attr_setstacksize(&attr, threadConfig.stackSize);
      if (ret != 0)
        {
          DAWNERR(
            "Failed to set worker thread stack size to %zu (%d)\n", threadConfig.stackSize, ret);
          return -ret;
        }
    }

  if (threadConfig.priority == THREAD_PRIORITY_DEFAULT &&
      threadConfig.scheduler == THREAD_SCHEDULER_DEFAULT)
    {
      return OK;
    }

  ret = pthread_getschedparam(pthread_self(), &policy, &param);
  if (ret != 0)
    {
      DAWNERR("Failed to read creator thread scheduler (%d)\n", ret);
      return -ret;
    }

  if (threadConfig.scheduler != THREAD_SCHEDULER_DEFAULT)
    {
      policy = threadConfig.scheduler;
    }

  minPrio = sched_get_priority_min(policy);
  maxPrio = sched_get_priority_max(policy);
  if (minPrio < 0 || maxPrio < 0)
    {
      int err;

      err = errno != 0 ? errno : EINVAL;
      DAWNERR("Failed to query priority range for scheduler %d\n", policy);
      return -err;
    }

  priority = param.sched_priority;
  if (threadConfig.priority != THREAD_PRIORITY_DEFAULT)
    {
      priority = threadConfig.priority;
    }
  else if (priority < minPrio || priority > maxPrio)
    {
      priority = minPrio;
    }

  if (priority < minPrio || priority > maxPrio)
    {
      DAWNERR("Worker thread priority %d is outside scheduler %d range [%d, %d]\n",
              priority,
              policy,
              minPrio,
              maxPrio);
      return -EINVAL;
    }

  param.sched_priority = priority;

  ret = pthread_attr_setschedpolicy(&attr, policy);
  if (ret != 0)
    {
      DAWNERR("Failed to set worker thread scheduler %d (%d)\n", policy, ret);
      return -ret;
    }

  ret = pthread_attr_setschedparam(&attr, &param);
  if (ret != 0)
    {
      DAWNERR("Failed to set worker thread priority %d (%d)\n", priority, ret);
      return -ret;
    }

  ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
  if (ret != 0)
    {
      DAWNERR("Failed to enable explicit worker thread scheduling (%d)\n", ret);
      return -ret;
    }

  return OK;
}

int CThreadedObject::threadStart()
{
  pthread_attr_t attr;
  bool needsDestroy;
  int ret;

  if (thCreated)
    {
      if (!thQuitDone)
        {
          DAWNERR("Thread already running\n");
          return -EBUSY;
        }

      ret = joinThread();
      if (ret != OK)
        {
          return ret;
        }
    }

  if (!threadFunc)
    {
      DAWNERR("Thread function not set before calling threadStart()\n");
      return -EINVAL;
    }

  thQuit = false;
  thQuitDone = false;

  ret = buildThreadAttr(attr, needsDestroy);
  if (ret != OK)
    {
      if (needsDestroy)
        {
          int destroyRet;

          destroyRet = pthread_attr_destroy(&attr);
          if (destroyRet != 0)
            {
              DAWNERR("Failed to destroy worker thread attributes (%d)\n", destroyRet);
            }
        }

      thQuit = true;
      thQuitDone = true;
      return ret;
    }

  ret = pthread_create(&th, needsDestroy ? &attr : nullptr, &CThreadedObject::threadEntry, this);

  if (needsDestroy)
    {
      int destroyRet;

      destroyRet = pthread_attr_destroy(&attr);
      if (destroyRet != 0)
        {
          DAWNERR("Failed to destroy worker thread attributes (%d)\n", destroyRet);
        }
    }

  if (ret != 0)
    {
      DAWNERR("Failed to create worker thread (%d)\n", ret);
      thQuit = true;
      thQuitDone = true;
      return -ret;
    }

  thCreated = true;
  return OK;
}

int CThreadedObject::threadStop()
{
  int ret;

  thQuit = true;

  ret = joinThread();
  if (ret != OK)
    {
      return ret;
    }

  thQuitDone = true;
  return OK;
}

bool CThreadedObject::isRunning() const
{
  return !thQuitDone;
}

void CThreadedObject::threadWrapper()
{
  if (threadFunc)
    {
      threadFunc();
    }

  thQuitDone = true;
}

void *CThreadedObject::threadEntry(void *arg)
{
  CThreadedObject *self;

  self = static_cast<CThreadedObject *>(arg);
  if (self != nullptr)
    {
      self->threadWrapper();
    }

  return nullptr;
}
