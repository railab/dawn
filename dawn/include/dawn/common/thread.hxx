// dawn/include/dawn/common/thread.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <thread>
#include <utility>

#include "dawn/porting/config.hxx"

namespace dawn
{
/**
 * @brief Portable thread owner abstraction for Dawn components.
 *
 * Provides the low-level thread lifecycle API used by composed thread owners
 * and the convenience helpers previously exposed by the mixin wrapper.
 */

class CThreadedObject
{
public:
  /**
   * @brief Constructor - initializes thread management state.
   */

  CThreadedObject()
    : th(nullptr)
    , thQuit(true)
    , thQuitDone(true)
    , threadFunc(nullptr)
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
    return th != nullptr;
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
  std::thread *th;                  ///< Pointer to the worker std::thread object.
                                    ///< Managed by threadStart()/threadStop().
  std::atomic_bool thQuit;          ///< Quit signal flag for thread function
  std::atomic_bool thQuitDone;      ///< Thread completion confirmation flag

private:
  std::function<void()> threadFunc; ///< Stored thread function callback

  /** @brief Internal thread entry point wrapper */

  void threadWrapper();
};

} // Namespace dawn
