// dawn/src/io/notify_manager.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/notify_manager.hxx"

#include <new>

using namespace dawn;

CIONotifierManager::CIONotifierManager()
{
}

CIONotifierManager::~CIONotifierManager()
{
  // Stop all notifiers

  stop();

  // Delete all notifier instances

  for (auto &e : entries)
    {
      switch (e.type)
        {
          case CIOCommon::IO_NOTIFY_POLL:
            {
              delete e.poll;
              break;
            }

#ifdef CONFIG_DAWN_IO_NOTIFY_STREAM
          case CIOCommon::IO_NOTIFY_STREAM:
            {
              delete e.stream;
              break;
            }
#endif

          default:
            {
              break;
            }
        }
    }

  entries.clear();
}

CIONotifier *CIONotifierManager::findPoll(int prio)
{
  for (auto &e : entries)
    {
      if (e.type == CIOCommon::IO_NOTIFY_POLL && e.prio == prio)
        {
          return e.poll;
        }
    }

  return nullptr;
}

int CIONotifierManager::regIO(CIOCommon *io)
{
  uint8_t type;
  int prio;

  if (io == nullptr)
    {
      return -EINVAL;
    }

  // Only register notifiable IOs with valid file descriptors

  if (!io->isNotify() || io->getFd() < 0)
    {
      return OK;
    }

  // Get IO notifier configuration

  type = io->getNotifyType();
  prio = io->getNotifyPrio();

  switch (type)
    {
      case CIOCommon::IO_NOTIFY_POLL:
        {
          CIONotifier *notifier;
          SNotifierEntry entry;

          // Find or create notifier for this priority

          notifier = findPoll(prio);
          if (notifier == nullptr)
            {
              notifier = new (std::nothrow) CIONotifier();
              if (notifier == nullptr)
                {
                  DAWNERR("Failed to allocate poll notifier\n");
                  return -ENOMEM;
                }

              notifier->setThreadPriority(prio);

              entry.type = type;
              entry.prio = prio;
              entry.poll = notifier;
              entries.push_back(entry);
            }

          // Bind notifier to IO

          io->bindNotifier(notifier);
          break;
        }

#ifdef CONFIG_DAWN_IO_NOTIFY_STREAM
      case CIOCommon::IO_NOTIFY_STREAM:
        {
          CStreamNotifier *notifier;
          SNotifierEntry entry;

          // Always create new stream notifier (one per IO)

          notifier = new (std::nothrow) CStreamNotifier();
          if (notifier == nullptr)
            {
              DAWNERR("Failed to allocate stream notifier\n");
              return -ENOMEM;
            }

          notifier->setThreadPriority(prio);

          entry.type = type;
          entry.prio = prio;
          entry.stream = notifier;
          entries.push_back(entry);

          // Bind notifier to IO

          io->bindNotifier(notifier);
          break;
        }
#endif

      default:
        {
          DAWNERR("Unsupported notifier type %d\n", type);
          return -ENOTSUP;
        }
    }

  // Register dummy notification to allocate data buffer and poll entry

  io->setNotifier(nullptr, 0, 0);

  return OK;
}

int CIONotifierManager::start()
{
  int ret;

  for (auto &e : entries)
    {
      switch (e.type)
        {
          case CIOCommon::IO_NOTIFY_POLL:
            {
              ret = e.poll->start();
              break;
            }

#ifdef CONFIG_DAWN_IO_NOTIFY_STREAM
          case CIOCommon::IO_NOTIFY_STREAM:
            {
              ret = e.stream->start();
              break;
            }
#endif

          default:
            {
              ret = -ENOTSUP;
              break;
            }
        }

      if (ret != OK)
        {
          DAWNERR("Failed to start notifier (type=%d prio=%d err=%d)\n", e.type, e.prio, ret);
          return ret;
        }
    }

  return OK;
}

int CIONotifierManager::stop()
{
  for (auto &e : entries)
    {
      switch (e.type)
        {
          case CIOCommon::IO_NOTIFY_POLL:
            {
              e.poll->stop();
              break;
            }

#ifdef CONFIG_DAWN_IO_NOTIFY_STREAM
          case CIOCommon::IO_NOTIFY_STREAM:
            {
              e.stream->stop();
              break;
            }
#endif

          default:
            {
              break;
            }
        }
    }

  return OK;
}
