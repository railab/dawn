// dawn/include/dawn/common/poll_loop.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <poll.h>

#include "dawn/common/thread.hxx"

namespace dawn
{
/**
 * @brief Default poll timeout for worker thread loops (milliseconds).
 *
 * Threads use a bounded poll timeout so they can periodically check quit
 * requests. Blocking forever can prevent threadStop() from completing.
 */

static constexpr int DAWN_POLL_TIMEOUT_MS = 1000;

/**
 * @brief Callback set for poll-based worker loops.
 */

struct SPollLoopCallbacks
{
  /**
   * @brief Optional hook called before each poll() call.
   */

  int (*beforePoll)(void *priv, struct pollfd *pfds, nfds_t nfds);

  /**
   * @brief Optional hook called after each poll() return.
   */

  void (*afterPoll)(void *priv, struct pollfd *pfds, nfds_t nfds, int ret);

  /**
   * @brief Optional hook called when poll() reports ready descriptors.
   */

  int (*onPollReady)(void *priv, struct pollfd *pfds, nfds_t nfds, int pollRet);
};

/**
 * @brief Shared runner for poll-based worker threads.
 */

class CPollLoopRunner
{
public:
  /**
   * @brief Run poll loop until quit is requested.
   *
   * The loop periodically wakes by timeout so quit requests can be observed.
   */

  static int run(CThreadedObject &threadCtl,
                 struct pollfd *pfds,
                 nfds_t nfds,
                 int timeoutMs,
                 const SPollLoopCallbacks &callbacks,
                 void *priv)
  {
    int ret;

    do
      {
        if (callbacks.beforePoll)
          {
            ret = callbacks.beforePoll(priv, pfds, nfds);
            if (ret < 0)
              {
                continue;
              }
          }

        ret = poll(pfds, nfds, timeoutMs);

        if (callbacks.afterPoll)
          {
            callbacks.afterPoll(priv, pfds, nfds, ret);
          }

        if (ret > 0 && callbacks.onPollReady)
          {
            callbacks.onPollReady(priv, pfds, nfds, ret);
          }
      }
    while (!threadCtl.shouldQuit());

    threadCtl.markThreadFinished();
    return 0;
  }
};

} // Namespace dawn
