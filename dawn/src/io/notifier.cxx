// dawn/src/io/notifier.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/notifier.hxx"

#include <new>

#include "dawn/common/poll_loop.hxx"
#include "dawn/io/common.hxx"

using namespace dawn;

// Update poll array.

void CIONotifier::updatePfds()
{
  size_t i = 0;

  pfdsLock.lock();

  for (const SIONotifierPriv &tmp : vnote)
    {
      bool found = false;
      int fd;

      // Check for at least one valid callback

      for (const SIONotifier &n : tmp.user)
        {
          if (n.cb)
            {
              found = true;
              break;
            }
        }

      fd = tmp.user[0].io->getFd();

      // Add to poll array if callback is registered and FD is valid

      if (found && fd >= 0)
        {
          pfds[i].fd = fd;
          pfds[i].events = POLLIN;
          pfds[i].revents = 0;
        }
      else
        {
          if (fd < 0)
            {
              DAWNERR("Invalid FD %d from IO 0x%08" PRIx32 ", skipping notifier\n",
                      fd,
                      tmp.user[0].io->getIdV());
            }

          pfds[i].fd = -1;
        }

      // Next item

      i++;
    }

  pfdsLock.unlock();
}

void CIONotifier::thread()
{
  SPollLoopCallbacks callbacks;

  struct
  {
    CIONotifier *self;
  } ctx;

  ctx.self = this;

  /* Loop until stop called */

  callbacks.beforePoll = [](void *priv, struct pollfd *pollfds, nfds_t nfds) -> int
    {
      CIONotifier *self;

      if (priv == nullptr)
        {
          return -EINVAL;
        }

      self = static_cast<decltype(ctx) *>(priv)->self;
      if (self == nullptr)
        {
          return -EINVAL;
        }

      (void)pollfds;
      (void)nfds;

      // Update poll array if requested
      if (self->pfdsUpdate.load())
        {
          self->updatePfds();
          self->pfdsUpdate = false;
        }

      return OK;
    };

  callbacks.afterPoll = [](void *priv, struct pollfd *pollfds, nfds_t nfds, int ret)
    {
      (void)priv;
      (void)pollfds;
      (void)nfds;
      (void)ret;
    };

  callbacks.onPollReady = [](void *priv, struct pollfd *pollfds, nfds_t nfds, int pollRet) -> int
    {
      CIONotifier *self;
      size_t i;
      int j;

      if (priv == nullptr || pollfds == nullptr || nfds == 0 || pollRet <= 0)
        {
          return -EINVAL;
        }

      self = static_cast<decltype(ctx) *>(priv)->self;
      if (self == nullptr)
        {
          return -EINVAL;
        }

      j = 0;

      // Lock poll data until we handle all requests
      self->pfdsLock.lock();

      for (i = 0; i < self->pfdsLen; i++)
        {
          if (pollfds[i].revents & POLLIN)
            {
              // Read data with configured batch count
              self->vnote[i].user[0].io->getData(*self->vnote[i].data, self->vnote[i].batch);

              for (SIONotifier &user : self->vnote[i].user)
                {
                  if (user.cb)
                    {
                      user.cb(user.priv, self->vnote[i].data);
                    }
                }

              pollfds[i].revents = 0;
              j++;
            }

          // poll() returns the count of fds with events; once we've handled
          // that many we can stop scanning the rest.
          if (j >= pollRet)
            {
              break;
            }
        }

      self->pfdsLock.unlock();
      return OK;
    };

  CPollLoopRunner::run(threadCtl, pfds, pfdsLen, DAWN_POLL_TIMEOUT_MS, callbacks, &ctx);
}

CIONotifier::~CIONotifier()
{
  // Make sure thread is stopped

  stop();

  // Clear flag

  pfdsUpdate = false;

  // Free allocated data

  for (const auto &n : vnote)
    {
      if (n.data)
        {
          delete n.data;
        }
    }

  // Clear registered notifications

  vnote.clear();

  // Free poll array

  delete[] pfds;
}

int CIONotifier::start()
{
  // Allocate poll array

  pfdsLen = vnote.size();
  pfds = new (std::nothrow) pollfd[pfdsLen]();
  if (pfds == nullptr)
    {
      DAWNERR("Failed to allocate poll array\n");
      return -ENOMEM;
    }

  // Initialize notifiers

  updatePfds();

  // Start notifier thread

  threadCtl.setThreadFunc([this]() { thread(); });
  return threadCtl.threadStart();
}

int CIONotifier::stop()
{
  // Stop notifier thread

  return threadCtl.threadStop();
}

int CIONotifier::regNotifier(SIONotifier n)
{
  SIONotifierPriv np;
  int update = -1;
  int i = 0;

  if (n.io == nullptr)
    {
      DAWNERR("IO pointer is null\n");
      return -EINVAL;
    }

  if (!n.io->isNotify())
    {
      DAWNERR("isNotify=false for IO 0x%08" PRIx32 "\n", n.io->getIdV());
      return -EINVAL;
    }

  // Check if we need update old entry

  for (const auto &v : vnote)
    {
      if (n.io == v.user[0].io)
        {
          update = i;
          break;
        }

      i++;
    }

  pfdsLock.lock();

  // Update vnote entry or add to vector

  if (update != -1)
    {
      vnote[update].user.push_back(n);

      pfdsUpdate = true;
    }
  else
    {
      // regNotifier() can be called only before start was called
      // update vnote when thread is running is not supported now

      if (threadCtl.hasThreadObject())
        {
          DAWNERR("Cannot register new notifier after thread started\n");
          pfdsLock.unlock();
          return -EPERM;
        }

      np.user.push_back(n);

      // Allocate data handler with configured batch count

      np.batch = n.io->getNotifyBatch();
      np.data = n.io->ddata_alloc(np.batch);
      if (np.data == nullptr)
        {
          DAWNERR("Failed to allocate data for notifier\n");
          pfdsLock.unlock();
          return -ENOMEM;
        }

      vnote.push_back(np);
    }

  pfdsLock.unlock();

  return OK;
}

int CIONotifier::notifyData(CIOCommon *io, io_ddata_t *data)
{
  if (io == nullptr || data == nullptr)
    {
      return -EINVAL;
    }

  pfdsLock.lock();

  for (auto &entry : vnote)
    {
      if (entry.user.empty() || entry.user[0].io != io)
        {
          continue;
        }

      for (SIONotifier &user : entry.user)
        {
          if (user.cb)
            {
              user.cb(user.priv, data);
            }
        }

      pfdsLock.unlock();
      return OK;
    }

  pfdsLock.unlock();
  return -ENOENT;
}
