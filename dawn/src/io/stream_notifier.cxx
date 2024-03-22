// dawn/src/io/stream_notifier.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/stream_notifier.hxx"

#include <new>

#include "dawn/common/poll_loop.hxx"
#include "dawn/io/common.hxx"

using namespace dawn;

CStreamNotifier::CStreamNotifier()
  : io(nullptr)
  , data(nullptr)
  , batch(1)
  , threadCtl()
{
}

CStreamNotifier::~CStreamNotifier()
{
  // Make sure thread is stopped

  stop();

  // Free allocated data

  if (data)
    {
      delete data;
    }

  // Clear registered callbacks

  users.clear();
}

void CStreamNotifier::thread()
{
  SPollLoopCallbacks callbacks;
  struct pollfd pfd;

  struct
  {
    CStreamNotifier *self;
  } ctx;

  ctx.self = this;

  // Set up single file descriptor for polling

  pfd.fd = io->getFd();
  pfd.events = POLLIN;
  pfd.revents = 0;

  // No pre/post-poll hooks needed for single-fd stream

  callbacks.beforePoll = nullptr;
  callbacks.afterPoll = nullptr;

  callbacks.onPollReady = [](void *priv, struct pollfd *pollfds, nfds_t nfds, int pollRet) -> int
    {
      CStreamNotifier *self;

      (void)pollfds;
      (void)nfds;
      (void)pollRet;

      if (priv == nullptr)
        {
          return -EINVAL;
        }

      self = static_cast<decltype(ctx) *>(priv)->self;
      if (self == nullptr)
        {
          return -EINVAL;
        }

      // Read data with configured batch count

      self->io->getData(*self->data, self->batch);

      // Dispatch to all registered callbacks

      for (SIONotifier &user : self->users)
        {
          if (user.cb)
            {
              user.cb(user.priv, self->data);
            }
        }

      return OK;
    };

  CPollLoopRunner::run(threadCtl, &pfd, 1, DAWN_POLL_TIMEOUT_MS, callbacks, &ctx);
}

int CStreamNotifier::start()
{
  if (io == nullptr)
    {
      DAWNERR("No IO bound to stream notifier\n");
      return -EINVAL;
    }

  // Start notifier thread

  threadCtl.setThreadFunc([this]() { thread(); });
  return threadCtl.threadStart();
}

int CStreamNotifier::stop()
{
  // Stop notifier thread

  return threadCtl.threadStop();
}

int CStreamNotifier::regNotifier(SIONotifier n)
{
  if (n.io == nullptr)
    {
      DAWNERR("IO pointer is null\n");
      return -EINVAL;
    }

  if (!n.io->isNotify())
    {
      DAWNERR("isNotify=false!\n");
      return -EINVAL;
    }

  // Stream notifier is 1:1 with IO

  if (io != nullptr && io != n.io)
    {
      DAWNERR("Stream notifier already bound to different IO\n");
      return -EINVAL;
    }

  // First registration: bind IO and allocate data buffer

  if (io == nullptr)
    {
      if (threadCtl.hasThreadObject())
        {
          DAWNERR("Cannot bind new IO after thread started\n");
          return -EPERM;
        }

      io = n.io;
      batch = n.io->getNotifyBatch();
      data = n.io->ddata_alloc(batch);
      if (data == nullptr)
        {
          DAWNERR("Failed to allocate data for stream notifier\n");
          io = nullptr;
          return -ENOMEM;
        }
    }

  // Append callback

  users.push_back(n);

  return OK;
}
