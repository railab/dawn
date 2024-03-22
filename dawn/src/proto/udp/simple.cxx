// dawn/src/proto/udp/simple.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/udp/simple.hxx"

#include "dawn/common/poll_loop.hxx"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>

#include <cstring>

using namespace dawn;

int CProtoUdp::sendFrame(uint8_t cmd, const uint8_t *payload, size_t len)
{
  uint8_t frame[FRAME_MAX_PAYLOAD + FRAME_MIN_LEN];
  size_t tosend;
  ssize_t ret;
  uint16_t crc;

  if (len > FRAME_MAX_PAYLOAD || fd < 0)
    {
      DAWNERR("UDP sendFrame invalid state cmd=0x%02x len=%zu fd=%d\n", cmd, len, fd);
      return -1;
    }

  tosend = FRAME_MIN_LEN + len;

  frame[0] = FRAME_SYNC;
  frame[1] = (uint8_t)(len & 0xFF);
  frame[2] = (uint8_t)((len >> 8) & 0xFF);
  frame[3] = cmd;

  if (len > 0)
    {
      std::memcpy(&frame[4], payload, len);
    }

  crc = calculateCrc(&frame[3], 1 + len);
  frame[4 + len] = (uint8_t)(crc & 0xFF);
  frame[5 + len] = (uint8_t)((crc >> 8) & 0xFF);

  {
    std::lock_guard<std::mutex> lock(senderMutex);
    if (sender.sin_port == 0)
      {
        DAWNERR("UDP sendFrame no sender cmd=0x%02x len=%zu\n", cmd, len);
        return -1;
      }

    DAWNINFO("UDP sendFrame cmd=0x%02x len=%zu to port=%u ip=0x%08" PRIx32 "\n",
             cmd,
             len,
             ntohs(sender.sin_port),
             ntohl(sender.sin_addr.s_addr));

    ret =
      sendto(fd, frame, tosend, 0, reinterpret_cast<struct sockaddr *>(&sender), sizeof(sender));
  }

  if (ret < 0 || static_cast<size_t>(ret) != tosend)
    {
      DAWNERR("UDP sendto failed cmd=0x%02x ret=%zd exp=%zu errno=%d\n", cmd, ret, tosend, errno);
      return ret < 0 ? static_cast<int>(ret) : -EIO;
    }

  return OK;
}

void CProtoUdp::thread()
{
  SPollLoopCallbacks callbacks;
  struct pollfd fds[1];

  std::memset(fds, 0, sizeof(fds));

  fds[0].fd = fd;
  fds[0].events = POLLIN;

  callbacks.beforePoll = CProtoUdp::cbPollBefore;
  callbacks.afterPoll = CProtoUdp::cbPollAfter;
  callbacks.onPollReady = CProtoUdp::cbPollOnReady;

  CPollLoopRunner::run(workerThread(), fds, 1, DAWN_POLL_TIMEOUT_MS, callbacks, this);
}

int CProtoUdp::pollBefore(struct pollfd *pfds, nfds_t nfds)
{
  if (pfds == nullptr || nfds == 0)
    {
      return -EINVAL;
    }

  pfds[0].revents = 0;
  return OK;
}

void CProtoUdp::pollAfter(int ret)
{
  if (ret < 0)
    {
      DAWNERR("UDP poll failed %d\n", -errno);
    }
}

int CProtoUdp::pollOnReady(struct pollfd *pfds, nfds_t nfds, int pollRet)
{
  socklen_t fromlen;
  ssize_t ret;

  if (pfds == nullptr || nfds == 0 || pollRet <= 0)
    {
      return -EINVAL;
    }

  if ((pfds[0].revents & POLLIN) == 0)
    {
      return OK;
    }

  fromlen = sizeof(sender);
  {
    std::lock_guard<std::mutex> lock(senderMutex);
    ret = recvfrom(
      fd, rxbuffer, sizeof(rxbuffer), 0, reinterpret_cast<struct sockaddr *>(&sender), &fromlen);
  }

  if (ret <= 0)
    {
      DAWNERR("UDP recvfrom ret=%zd errno=%d\n", ret, errno);
      usleep(10000);
      return static_cast<int>(ret);
    }

  int frameRet = handleFrame(rxbuffer, static_cast<size_t>(ret));
  if (frameRet < 0)
    {
      DAWNERR("UDP handleFrame failed ret=%d\n", frameRet);
    }

  pfds[0].revents = 0;
  return OK;
}

int CProtoUdp::cbPollBefore(void *priv, struct pollfd *pfds, nfds_t nfds)
{
  CProtoUdp *self;

  self = static_cast<CProtoUdp *>(priv);
  if (self == nullptr)
    {
      return -EINVAL;
    }

  return self->pollBefore(pfds, nfds);
}

void CProtoUdp::cbPollAfter(void *priv, struct pollfd *pfds, nfds_t nfds, int ret)
{
  CProtoUdp *self;

  (void)pfds;
  (void)nfds;

  self = static_cast<CProtoUdp *>(priv);
  if (self == nullptr)
    {
      return;
    }

  self->pollAfter(ret);
}

int CProtoUdp::cbPollOnReady(void *priv, struct pollfd *pfds, nfds_t nfds, int pollRet)
{
  CProtoUdp *self;

  self = static_cast<CProtoUdp *>(priv);
  if (self == nullptr)
    {
      return -EINVAL;
    }

  return self->pollOnReady(pfds, nfds, pollRet);
}

int CProtoUdp::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset;

  offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_UDP)
        {
          DAWNERR("Unsupported UDP config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_UDP_CFG_IOBIND:
            {
              for (size_t j = 0; j < item->cfgid.s.size;)
                {
                  SProtoUdpIOBind *tmp;

                  tmp = reinterpret_cast<SProtoUdpIOBind *>(item->data + j);

                  allocObject(tmp);
                  j += sizeof(SProtoUdpIOBind) / 4;
                }

              break;
            }

          case PROTO_UDP_CFG_PORT:
            {
              port = static_cast<uint16_t>(item->data[0]);
              break;
            }

          default:
            {
              DAWNERR("Unsupported UDP config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CProtoUdp::udpInit()
{
  struct sockaddr_in addr;
  int ret;

  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0)
    {
      DAWNERR("Failed to create UDP socket: %d\n", -errno);
      return -1;
    }

  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(port);

  ret = bind(fd, reinterpret_cast<struct sockaddr *>(&addr), sizeof(addr));
  if (ret < 0)
    {
      DAWNERR("Failed to bind UDP socket to port %u: %d\n", port, -errno);
      close(fd);
      fd = -1;
      return -1;
    }

  DAWNINFO("UDP protocol initialized on port %u\n", port);

  return 0;
}

CProtoUdp::~CProtoUdp()
{
  deinit();
}

int CProtoUdp::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("UDP configure failed (error %d)\n", ret);
      return ret;
    }

  return OK;
}

int CProtoUdp::init()
{
  int ret;

  ret = udpInit();
  if (ret < 0)
    {
      return ret;
    }

  ret = createBuffers();
  if (ret < 0)
    {
      DAWNERR("failed to create data %d\n", ret);
      return ret;
    }

#ifdef CONFIG_DAWN_IO_NOTIFY
  ret = setupNotifications();
  if (ret < 0)
    {
      DAWNERR("failed to setup notifications %d\n", ret);
    }
#endif

  return OK;
}

int CProtoUdp::deinit()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  destroyNotifications();
#endif

  destroyBuffers();

  if (fd >= 0)
    {
      close(fd);
      fd = -1;
    }

  return OK;
}

int CProtoUdp::doStart()
{
  int ret;

  ret = startWorkerThread([this]() { thread(); });
  if (ret < 0)
    {
      DAWNERR("failed to start thread %d\n", ret);
      return ret;
    }

  DAWNINFO("UDP protocol started\n");

  return 0;
}

int CProtoUdp::doStop()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  cleanupNotifications();
#endif

  stopWorkerThread();

  DAWNINFO("UDP protocol stopped\n");

  return 0;
}

bool CProtoUdp::hasThread() const
{
  return workerThreadRunning();
}
