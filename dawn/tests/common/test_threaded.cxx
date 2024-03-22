// dawn/tests/common/test_threaded.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <atomic>
#include <chrono>
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
  {
  }

  int start()
  {
    started = false;
    quitSeen = false;
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

  bool didSeeQuit() const
  {
    return quitSeen.load();
  }

  uint32_t getLoopCount() const
  {
    return loopCount.load();
  }

  int startAndExit()
  {
    started = false;
    quitSeen = false;
    loopCount = 0;
    return startWorkerThread([this]() { runAndExit(); });
  }

private:
  void run()
  {
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
    started = true;
    loopCount.fetch_add(1);
    quitSeen = true;
  }

  std::atomic_bool started;
  std::atomic_bool quitSeen;
  std::atomic_uint32_t loopCount;
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

extern "C"
{
  int test_common_threaded()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_common_threaded_lifecycle);
    DAWN_RUN_TEST(test_common_threaded_restart);
    DAWN_RUN_TEST(test_common_threaded_natural_exit_restart);
    return UNITY_END();
  }
}
