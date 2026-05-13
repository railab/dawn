// dawn/include/dawn/common/thread.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <pthread.h>
#include <utility>

#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Portable thread owner abstraction for Dawn components.
 *
 * Provides the low-level thread lifecycle API used by composed thread owners
 * and the convenience helpers previously exposed by the mixin wrapper.
 *
 * The worker thread is implemented with POSIX pthreads so Dawn can control
 * stack size, scheduling policy, and priority on a per-thread basis.
 * When no configuration is supplied, the OS default stack size and the
 * creating thread's scheduler settings are used.
 */

class CThreadedObject
{
public:
  /**
   * @brief Default thread priority behavior.
   *
   * Value 0 keeps the creator thread's default priority.
   */

  static constexpr int THREAD_PRIORITY_DEFAULT = 0;

  /**
   * @brief Default scheduler behavior.
   *
   * Value -1 keeps the creator thread's current scheduling policy.
   */

  static constexpr int THREAD_SCHEDULER_DEFAULT = -1;

  /**
   * @brief Per-thread runtime configuration.
   */

  struct SThreadConfig
  {
    size_t stackSize; ///< Requested stack size in bytes (0 = OS default).
    int priority;     ///< Requested thread priority (0 = creator default).
    int scheduler;    ///< Requested scheduler policy (-1 = creator default).

    constexpr SThreadConfig(size_t stackSizeBytes = 0,
                            int priorityValue = THREAD_PRIORITY_DEFAULT,
                            int schedulerPolicy = THREAD_SCHEDULER_DEFAULT)
      : stackSize(stackSizeBytes)
      , priority(priorityValue)
      , scheduler(schedulerPolicy)
    {
    }
  };

  /**
   * @brief Constructor - initializes thread management state.
   */

  CThreadedObject()
    : th()
    , thCreated(false)
    , thQuit(true)
    , thQuitDone(true)
    , threadFunc(nullptr)
    , threadConfig()
  {
  }

  CThreadedObject(const CThreadedObject &) = delete;
  CThreadedObject &operator=(const CThreadedObject &) = delete;

  /**
   * @brief Destructor - cleans up thread resources.
   */

  virtual ~CThreadedObject();

  /**
   * @brief Assign the function executed by threadStart().
   *
   * @param func Callable object to execute in the worker thread.
   */

  template<typename Func>
  void setThreadFunc(Func &&func)
  {
    threadFunc = std::forward<Func>(func);
  }

  /**
   * @brief Replace the full thread configuration.
   */

  void setThreadConfig(const SThreadConfig &config)
  {
    threadConfig = config;
  }

  /**
   * @brief Get current thread configuration.
   */

  const SThreadConfig &getThreadConfig() const
  {
    return threadConfig;
  }

  /**
   * @brief Configure worker thread stack size.
   *
   * @param stackSize Requested stack size in bytes (0 = OS default).
   */

  void setThreadStackSize(size_t stackSize)
  {
    threadConfig.stackSize = stackSize;
  }

  /**
   * @brief Get configured worker thread stack size.
   */

  size_t getThreadStackSize() const
  {
    return threadConfig.stackSize;
  }

  /**
   * @brief Configure worker thread priority.
   *
   * @param priority Requested priority (0 = creator default).
   */

  void setThreadPriority(int priority)
  {
    threadConfig.priority = priority;
  }

  /**
   * @brief Get configured worker thread priority.
   */

  int getThreadPriority() const
  {
    return threadConfig.priority;
  }

  /**
   * @brief Configure worker thread scheduler policy.
   *
   * @param scheduler POSIX scheduler policy (-1 = creator default).
   */

  void setThreadScheduler(int scheduler)
  {
    threadConfig.scheduler = scheduler;
  }

  /**
   * @brief Get configured worker thread scheduler policy.
   */

  int getThreadScheduler() const
  {
    return threadConfig.scheduler;
  }

  /**
   * @brief Start the worker thread.
   *
   * @return OK on success, negative error code on failure.
   */

  int threadStart();

  /**
   * @brief Stop the worker thread.
   *
   * @return OK on success, negative error code on failure.
   */

  int threadStop();

  /**
   * @brief Check if the worker thread is running.
   *
   * @return True if thread is running, false otherwise.
   */

  bool isRunning() const;

  bool shouldQuit() const
  {
    return thQuit.load();
  }

  bool isStopped() const
  {
    return thQuitDone.load();
  }

  bool hasThreadObject() const
  {
    return thCreated;
  }

  void requestStop()
  {
    thQuit = true;
  }

  void clearStopRequest()
  {
    thQuit = false;
  }

  void markThreadFinished()
  {
    thQuitDone = true;
  }

protected:
  /**
   * @brief Start the worker thread with a given function.
   *
   * @param func Callable object to execute in thread.
   * @return OK on success, negative error code on failure.
   */

  template<typename Func>
  int startWorkerThread(Func &&func)
  {
    setThreadFunc(std::forward<Func>(func));
    return threadStart();
  }

  /**
   * @brief Stop the worker thread.
   *
   * @return OK on success, negative error code on failure.
   */

  int stopWorkerThread()
  {
    return threadStop();
  }

  /**
   * @brief Check if the worker thread is running.
   *
   * @return True if thread is running, false otherwise.
   */

  bool workerThreadRunning() const
  {
    return isRunning();
  }

  /**
   * @brief Get a reference to this thread controller.
   *
   * @return Reference to this object.
   */

  CThreadedObject &workerThread()
  {
    return *this;
  }

  /**
   * @brief Get a const reference to this thread controller.
   *
   * @return Const reference to this object.
   */

  const CThreadedObject &workerThread() const
  {
    return *this;
  }

private:
  pthread_t th;                     ///< Worker pthread handle.
  bool thCreated;                   ///< True when pthread_create() succeeded.
  std::atomic_bool thQuit;          ///< Quit signal flag for thread function.
  std::atomic_bool thQuitDone;      ///< Thread completion confirmation flag.
  std::function<void()> threadFunc; ///< Stored thread function callback.
  SThreadConfig threadConfig;       ///< Per-thread configuration.

  int joinThread();
  int buildThreadAttr(pthread_attr_t &attr, bool &needsDestroy) const;
  void threadWrapper();
  static void *threadEntry(void *arg);
};

} // Namespace dawn
