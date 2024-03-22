// dawn/src/proto/can/can.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "can_internal.hxx"

#include "dawn/common/poll_loop.hxx"
#include "dawn/io/common.hxx"
#include "dawn/porting/can.hxx"

using namespace dawn;

int CProtoCan::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_CAN)
        {
          DAWNERR("Unsupported CAN config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_CAN_CFG_IOBIND:
            {
              for (size_t j = 0; j < item->cfgid.s.size;)
                {
                  SProtoCanIOBind *tmp = reinterpret_cast<SProtoCanIOBind *>(item->data + j);

                  allocObject(tmp);

                  j += sizeof(SProtoCanIOBind) / 4 + tmp->size;
                }

              break;
            }

          case CProtoCan::PROTO_CAN_CFG_DEVNO:
            {
              devno = static_cast<uint32_t>(item->data[0]);
              break;
            }

          case CProtoCan::PROTO_CAN_CFG_NODEID:
            {
              nodeid = static_cast<uint32_t>(item->data[0]);
              break;
            }

          default:
            {
              DAWNERR("Unsupported CAN config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

void CProtoCan::allocObject(SProtoCanIOBind *alloc)
{
  for (size_t i = 0; i < alloc->size; i++)
    {
      DAWNINFO("allocate object 0x%" PRIx32 "\n", alloc->objid[i]);
      setObjectMapItem(alloc->objid[i], nullptr);
    }

  valloc.push_back(alloc);
};

void CProtoCan::thread()
{
  struct pollfd fds[1];
  SPollLoopCallbacks callbacks;

  DAWNINFO("start CAN thread\n");

  std::memset(fds, 0, sizeof(fds));

  fds[0].fd = fd;
  fds[0].events = POLLIN;

  callbacks.beforePoll = beforePoll;
  callbacks.afterPoll = afterPoll;
  callbacks.onPollReady = onPollReady;

  CPollLoopRunner::run(workerThread(), fds, 1, DAWN_POLL_TIMEOUT_MS, callbacks, this);
}

int CProtoCan::beforePoll(void *priv, struct pollfd *pfds, nfds_t nfds)
{
  (void)priv;

  if (pfds == nullptr || nfds == 0)
    {
      return -EINVAL;
    }

  pfds[0].revents = 0;
  return OK;
}

void CProtoCan::afterPoll(void *priv, struct pollfd *pfds, nfds_t nfds, int ret)
{
  (void)priv;
  (void)pfds;
  (void)nfds;

  if (ret < 0)
    {
      DAWNERR("CAN poll failed %d\n", -errno);
    }
}

int CProtoCan::onPollReady(void *priv, struct pollfd *pfds, nfds_t nfds, int pollRet)
{
  CProtoCan *self;
  dawn::porting::canmsg_s msg;
  int ret;

  if (priv == nullptr || pfds == nullptr || nfds == 0 || pollRet <= 0)
    {
      return -EINVAL;
    }

  if ((pfds[0].revents & POLLIN) == 0)
    {
      return OK;
    }

  self = static_cast<CProtoCan *>(priv);

  ret = can_read(self->fd, &msg);
  if (ret > 0)
    {
      ret = self->msgRecv(msg);
      if (ret < 0)
        {
          DAWNERR("Failed to handle msg %d\n", ret);
        }
    }

  pfds[0].revents = 0;
  return OK;
}

int CProtoCan::canInitialize()
{
  int ret;

  if (devno == static_cast<uint32_t>(-1))
    {
      DAWNERR("Invalid CAN device number configuration\n");
      return -EINVAL;
    }

  snprintf(path, sizeof(path), CAN_PATH_FMT, devno);

  fd = can_open(path);
  if (fd < 0)
    {
      DAWNERR("can_open failed %d\n", -errno);
      return -errno;
    }

  ret = can_init(fd);
  if (ret < 0)
    {
      DAWNERR("can_init failed %d\n", ret);
      return ret;
    }

  return OK;
}

CProtoCan::~CProtoCan()
{
  deinit();
}

int CProtoCan::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("CAN configure failed (error %d)\n", ret);
      return ret;
    }

  return OK;
}

int CProtoCan::init()
{
  int ret;

  ret = canInitialize();
  if (ret != OK)
    {
      return ret;
    }

  ret = createRegs();
  if (ret < 0)
    {
      DAWNERR("failed to create registers %d\n", ret);
      return ret;
    }

  return OK;
}

int CProtoCan::deinit()
{
  destroyRegs();

  if (fd >= 0)
    {
      can_close(fd);
      fd = -1;
    }

  return OK;
}

int CProtoCan::doStart()
{
  return startWorkerThread([this]() { thread(); });
};

int CProtoCan::doStop()
{
  return stopWorkerThread();
};

bool CProtoCan::hasThread() const
{
  return workerThreadRunning();
}
