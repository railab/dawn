// dawn/tests/common/test_threaded.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <atomic>
#include <chrono>
#include <pthread.h>
#include <sched.h>
#include <thread>

#include "dawn/common/thread.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: threaded mock exposes worker lifecycle state for tests.
//***************************************************************************

class CThreadedObjectMock : protected CThreadedObject
{
public:
  CThreadedObjectMock()
    : started(false)
    , quitSeen(false)
    , loopCount(0)
    , schedReadRet(-1)
    , observedScheduler(THREAD_SCHEDULER_DEFAULT)
    , observedPriority(THREAD_PRIORITY_DEFAULT)
    , schedulingCaptured(false)
  {
  }

  int start()
  {
    started = false;
    quitSeen = false;
    schedReadRet = -1;
    observedScheduler = THREAD_SCHEDULER_DEFAULT;
    observedPriority = THREAD_PRIORITY_DEFAULT;
    schedulingCaptured = false;
    return startWorkerThread([this]() { run(); });
  }

  int stop()
  {
    return stopWorkerThread();
  }

  bool hasThread() const
  {
    return workerThreadRunning();
  }

  void configureThread(size_t stackSize, int priority, int scheduler)
  {
    setThreadConfig(SThreadConfig(stackSize, priority, scheduler));
  }

  size_t getConfiguredStackSize() const
  {
    return getThreadStackSize();
  }

  int getConfiguredPriority() const
  {
    return getThreadPriority();
  }

  int getConfiguredScheduler() const
  {
    return getThreadScheduler();
  }

  bool waitUntilStarted(int timeoutMs = 200) const
  {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline)
      {
        if (started.load())
          {
            return true;
          }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

    return started.load();
  }

  bool waitUntilSchedulingCaptured(int timeoutMs = 200) const
  {
    auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

    while (std::chrono::steady_clock::now() < deadline)
      {
        if (schedulingCaptured.load())
          {
            return true;
          }

        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

    return schedulingCaptured.load();
  }

  bool didSeeQuit() const
  {
    return quitSeen.load();
  }

  uint32_t getLoopCount() const
  {
    return loopCount.load();
  }

  int getSchedReadRet() const
  {
    return schedReadRet.load();
  }

  int getObservedScheduler() const
  {
    return observedScheduler.load();
  }

  int getObservedPriority() const
  {
    return observedPriority.load();
  }

  int startAndExit()
  {
    started = false;
    quitSeen = false;
    loopCount = 0;
    schedReadRet = -1;
    observedScheduler = THREAD_SCHEDULER_DEFAULT;
    observedPriority = THREAD_PRIORITY_DEFAULT;
    schedulingCaptured = false;
    return startWorkerThread([this]() { runAndExit(); });
  }

private:
  void captureScheduling()
  {
    struct sched_param param;
    int policy;
    int ret;

    policy = THREAD_SCHEDULER_DEFAULT;
    param.sched_priority = THREAD_PRIORITY_DEFAULT;

    ret = pthread_getschedparam(pthread_self(), &policy, &param);
    schedReadRet = ret;
    observedScheduler = policy;
    observedPriority = param.sched_priority;
    schedulingCaptured = true;
  }

  void run()
  {
    captureScheduling();
    started = true;

    while (!workerThread().shouldQuit())
      {
        loopCount.fetch_add(1);
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
      }

    quitSeen = true;
  }

  void runAndExit()
  {
    captureScheduling();
    started = true;
    loopCount.fetch_add(1);
    quitSeen = true;
  }

  std::atomic_bool started;
  std::atomic_bool quitSeen;
  std::atomic_uint32_t loopCount;
  std::atomic_int schedReadRet;
  std::atomic_int observedScheduler;
  std::atomic_int observedPriority;
  std::atomic_bool schedulingCaptured;
};

//***************************************************************************
// Description: worker thread starts, observes quit, and stops cleanly.
//***************************************************************************

static void test_common_threaded_lifecycle()
{
  CThreadedObjectMock obj;
  int ret;

  TEST_ASSERT_FALSE(obj.hasThread());

  ret = obj.start();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_TRUE(obj.waitUntilStarted());
  TEST_ASSERT_TRUE(obj.hasThread());

  ret = obj.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_FALSE(obj.hasThread());
  TEST_ASSERT_TRUE(obj.didSeeQuit());
  TEST_ASSERT_TRUE(obj.getLoopCount() > 0);
}

//***************************************************************************
// Description: worker thread can be stopped and started again.
//***************************************************************************

static void test_common_threaded_restart()
{
  CThreadedObjectMock obj;
  int ret;

  ret = obj.start();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_TRUE(obj.waitUntilStarted());
  TEST_ASSERT_TRUE(obj.hasThread());

  ret = obj.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_FALSE(obj.hasThread());
  TEST_ASSERT_TRUE(obj.didSeeQuit());

  ret = obj.start();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_TRUE(obj.waitUntilStarted());
  TEST_ASSERT_TRUE(obj.hasThread());

  ret = obj.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_FALSE(obj.hasThread());
  TEST_ASSERT_TRUE(obj.didSeeQuit());
}

//***************************************************************************
// Description: naturally exited worker can be stopped and restarted.
//***************************************************************************

static void test_common_threaded_natural_exit_restart()
{
  CThreadedObjectMock obj;
  int ret;

  ret = obj.startAndExit();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_TRUE(obj.waitUntilStarted());
  TEST_ASSERT_FALSE(obj.hasThread());
  TEST_ASSERT_TRUE(obj.didSeeQuit());
  TEST_ASSERT_TRUE(obj.getLoopCount() > 0);

  ret = obj.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_FALSE(obj.hasThread());

  ret = obj.start();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_TRUE(obj.waitUntilStarted());
  TEST_ASSERT_TRUE(obj.hasThread());

  ret = obj.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_FALSE(obj.hasThread());
}

//***************************************************************************
// Description: thread configuration accessors preserve per-thread settings.
//***************************************************************************

static void test_common_threaded_config_accessors()
{
  CThreadedObjectMock obj;

  TEST_ASSERT_EQUAL((size_t)0, obj.getConfiguredStackSize());
  TEST_ASSERT_EQUAL(CThreadedObject::THREAD_PRIORITY_DEFAULT, obj.getConfiguredPriority());
  TEST_ASSERT_EQUAL(CThreadedObject::THREAD_SCHEDULER_DEFAULT, obj.getConfiguredScheduler());

  obj.configureThread(8192, 77, SCHED_FIFO);

  TEST_ASSERT_EQUAL((size_t)8192, obj.getConfiguredStackSize());
  TEST_ASSERT_EQUAL(77, obj.getConfiguredPriority());
  TEST_ASSERT_EQUAL(SCHED_FIFO, obj.getConfiguredScheduler());
}

//***************************************************************************
// Description: invalid scheduler and priority values are rejected.
//***************************************************************************

static void test_common_threaded_rejects_invalid_config()
{
  CThreadedObjectMock obj1;
  CThreadedObjectMock obj2;

  obj1.configureThread(
    0, CThreadedObject::THREAD_PRIORITY_DEFAULT, CThreadedObject::THREAD_SCHEDULER_DEFAULT - 1);
  TEST_ASSERT_EQUAL(-EINVAL, obj1.start());
  TEST_ASSERT_FALSE(obj1.hasThread());

  obj2.configureThread(
    0, CThreadedObject::THREAD_PRIORITY_DEFAULT - 1, CThreadedObject::THREAD_SCHEDULER_DEFAULT);
  TEST_ASSERT_EQUAL(-EINVAL, obj2.start());
  TEST_ASSERT_FALSE(obj2.hasThread());
}

//***************************************************************************
// Description: configured scheduler and priority are applied to worker thread.
//***************************************************************************

static void test_common_threaded_applies_priority_and_scheduler()
{
  CThreadedObjectMock obj;
  struct sched_param param;
  int currentPolicy;
  int desiredPriority;
  int maxPriority;
  int minPriority;
  int ret;

  ret = pthread_getschedparam(pthread_self(), &currentPolicy, &param);
  TEST_ASSERT_EQUAL(0, ret);

  minPriority = sched_get_priority_min(currentPolicy);
  maxPriority = sched_get_priority_max(currentPolicy);
  TEST_ASSERT_TRUE(minPriority >= 0);
  TEST_ASSERT_TRUE(maxPriority >= 0);

  desiredPriority = param.sched_priority;
  if (desiredPriority < maxPriority)
    {
      desiredPriority++;
    }
  else if (desiredPriority > minPriority)
    {
      desiredPriority--;
    }

  obj.configureThread(0, desiredPriority, currentPolicy);

  ret = obj.start();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_TRUE(obj.waitUntilStarted());
  TEST_ASSERT_TRUE(obj.waitUntilSchedulingCaptured());
  TEST_ASSERT_EQUAL(0, obj.getSchedReadRet());
  TEST_ASSERT_EQUAL(currentPolicy, obj.getObservedScheduler());
  TEST_ASSERT_EQUAL(desiredPriority, obj.getObservedPriority());

  ret = obj.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

extern "C"
{
  int test_common_threaded()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_common_threaded_lifecycle);
    DAWN_RUN_TEST(test_common_threaded_restart);
    DAWN_RUN_TEST(test_common_threaded_natural_exit_restart);
    DAWN_RUN_TEST(test_common_threaded_config_accessors);
    DAWN_RUN_TEST(test_common_threaded_rejects_invalid_config);
    DAWN_RUN_TEST(test_common_threaded_applies_priority_and_scheduler);
    return UNITY_END();
  }
}
