// dawn/src/proto/ipc/simple.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/ipc/simple.hxx"

#include "dawn/common/poll_loop.hxx"

#include <cerrno>
#include <cstring>

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <unistd.h>

using namespace dawn;

int CProtoIpc::sendFrame(uint8_t cmd, const uint8_t *payload, size_t len)
{
  uint8_t frame[FRAME_MAX_PAYLOAD + FRAME_MIN_LEN];
  size_t tosend;
  size_t sent;
  ssize_t ret;
  uint16_t crc;

  tosend = FRAME_MIN_LEN + len;
  sent = 0;

  if (len > FRAME_MAX_PAYLOAD || txfd < 0)
    {
      DAWNERR("IPC sendFrame invalid state cmd=0x%02x len=%zu fd=%d\n", cmd, len, txfd);
      return -EINVAL;
    }

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

  while (sent < tosend)
    {
      ret = write(txfd, &frame[sent], tosend - sent);
      if (ret < 0)
        {
          if (errno == EINTR)
            {
              continue;
            }

          DAWNERR("IPC write failed cmd=0x%02x ret=%d errno=%d\n", cmd, (int)ret, errno);
          return -errno;
        }

      if (ret == 0)
        {
          DAWNERR("IPC write returned zero bytes\n");
          return -EIO;
        }

      sent += (size_t)ret;
    }

  return OK;
}

void CProtoIpc::thread()
{
  struct pollfd fds[1];
  SPollLoopCallbacks callbacks;

  std::memset(fds, 0, sizeof(fds));

  fds[0].fd = rxfd;
  fds[0].events = POLLIN;
  parserPos = 0;
  parserLen = 0;
  parserState = 0;

  callbacks.beforePoll = CProtoIpc::cbPollBefore;
  callbacks.afterPoll = CProtoIpc::cbPollAfter;
  callbacks.onPollReady = CProtoIpc::cbPollOnReady;

  CPollLoopRunner::run(workerThread(), fds, 1, DAWN_POLL_TIMEOUT_MS, callbacks, this);
}

int CProtoIpc::pollBefore(struct pollfd *pfds, nfds_t nfds)
{
  if (pfds == nullptr || nfds == 0)
    {
      return -EINVAL;
    }

  pfds[0].revents = 0;
  return OK;
}

void CProtoIpc::pollAfter(int ret)
{
  if (ret < 0 && errno != EINTR)
    {
      DAWNERR("IPC poll failed %d\n", -errno);
    }
}

int CProtoIpc::pollOnReady(struct pollfd *pfds, nfds_t nfds, int pollRet)
{
  uint8_t chunk[64];
  ssize_t rdret;
  size_t i;
  uint8_t byte;
  int ret;

  if (pfds == nullptr || nfds == 0 || pollRet <= 0)
    {
      return -EINVAL;
    }

  if ((pfds[0].revents & (POLLIN | POLLERR | POLLHUP)) == 0)
    {
      return OK;
    }

  rdret = read(rxfd, chunk, sizeof(chunk));
  if (rdret < 0)
    {
      if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR)
        {
          return OK;
        }

      DAWNERR("IPC read failed ret=%d errno=%d\n", (int)rdret, errno);
      usleep(10000);
      return (int)rdret;
    }

  if (rdret == 0)
    {
      usleep(10000);
      return OK;
    }

  for (i = 0; i < (size_t)rdret; i++)
    {
      byte = chunk[i];

      switch (parserState)
        {
          case 0:
            {
              if (byte == FRAME_SYNC)
                {
                  rxbuffer[parserPos] = byte;
                  parserPos = 1;
                  parserLen = 0;
                  parserState = 1;
                }

              break;
            }

          case 1:
            {
              parserLen = byte;
              rxbuffer[parserPos] = byte;
              parserPos = 2;
              parserState = 2;
              break;
            }

          case 2:
            {
              parserLen |= (uint16_t)(byte << 8);
              if (parserLen > FRAME_MAX_PAYLOAD)
                {
                  parserState = 0;
                  parserPos = 0;
                  continue;
                }

              rxbuffer[parserPos] = byte;
              parserPos = 3;
              parserState = 3;
              break;
            }

          case 3:
            {
              rxbuffer[parserPos++] = byte;

              if (parserPos >= (size_t)(FRAME_MIN_LEN + parserLen))
                {
                  ret = handleFrame(rxbuffer, parserPos);
                  if (ret < 0)
                    {
                      DAWNERR("IPC handleFrame failed ret=%d\n", ret);
                    }

                  parserState = 0;
                  parserPos = 0;
                }

              break;
            }
        }
    }

  return OK;
}

int CProtoIpc::cbPollBefore(void *priv, struct pollfd *pfds, nfds_t nfds)
{
  CProtoIpc *self;

  self = static_cast<CProtoIpc *>(priv);
  if (self == nullptr)
    {
      return -EINVAL;
    }

  return self->pollBefore(pfds, nfds);
}

void CProtoIpc::cbPollAfter(void *priv, struct pollfd *pfds, nfds_t nfds, int ret)
{
  CProtoIpc *self;

  (void)pfds;
  (void)nfds;
  self = static_cast<CProtoIpc *>(priv);
  if (self == nullptr)
    {
      return;
    }

  self->pollAfter(ret);
}

int CProtoIpc::cbPollOnReady(void *priv, struct pollfd *pfds, nfds_t nfds, int pollRet)
{
  CProtoIpc *self;

  self = static_cast<CProtoIpc *>(priv);
  if (self == nullptr)
    {
      return -EINVAL;
    }

  return self->pollOnReady(pfds, nfds, pollRet);
}

int CProtoIpc::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset;

  offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_IPC)
        {
          DAWNERR("Unsupported IPC config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_IPC_CFG_IOBIND:
            {
              for (size_t j = 0; j < item->cfgid.s.size;)
                {
                  SProtoIpcIOBind *tmp;

                  tmp = reinterpret_cast<SProtoIpcIOBind *>(item->data + j);

                  allocObject(tmp);
                  j += sizeof(SProtoIpcIOBind) / 4;
                }

              break;
            }

          case PROTO_IPC_CFG_RX_PATH:
            {
              rxPath = reinterpret_cast<const char *>(&item->data);
              break;
            }

          case PROTO_IPC_CFG_TX_PATH:
            {
              txPath = reinterpret_cast<const char *>(&item->data);
              break;
            }

          default:
            {
              DAWNERR("Unsupported IPC config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CProtoIpc::ensureFifo(const char *path)
{
  struct stat st;
  int err;
  int ret;

  if (!path)
    {
      DAWNERR("IPC FIFO path not configured\n");
      return -EINVAL;
    }

  ret = mkfifo(path, 0666);
  if (ret == 0)
    {
      return OK;
    }

  err = errno;

  if (err == EEXIST)
    {
      ret = stat(path, &st);
      if (ret == 0 && S_ISFIFO(st.st_mode))
        {
          return OK;
        }

      if (ret < 0)
        {
          err = errno;
        }
      else
        {
          err = EINVAL;
        }
    }

  if (err <= 0)
    {
      err = EIO;
    }

  DAWNERR("Failed to create FIFO %s errno=%d\n", path, err);

  return -err;
}

int CProtoIpc::fifoInit()
{
  int ret;

  ret = ensureFifo(rxPath);
  if (ret < 0)
    {
      return ret;
    }

  ret = ensureFifo(txPath);
  if (ret < 0)
    {
      return ret;
    }

  rxfd = open(rxPath, O_RDWR | O_NONBLOCK);
  if (rxfd < 0)
    {
      DAWNERR("Failed to open IPC RX FIFO %s errno=%d\n", rxPath, errno);
      return -errno;
    }

  txfd = open(txPath, O_RDWR | O_NONBLOCK);
  if (txfd < 0)
    {
      DAWNERR("Failed to open IPC TX FIFO %s errno=%d\n", txPath, errno);
      close(rxfd);
      rxfd = -1;
      return -errno;
    }

  DAWNINFO("IPC protocol initialized rx=%s tx=%s\n", rxPath, txPath);

  return OK;
}

CProtoIpc::~CProtoIpc()
{
  deinit();
}

int CProtoIpc::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("IPC configure failed (error %d)\n", ret);
      return ret;
    }

  return OK;
}

int CProtoIpc::init()
{
  int ret;

  ret = fifoInit();
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

int CProtoIpc::deinit()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  destroyNotifications();
#endif

  destroyBuffers();

  if (rxfd >= 0)
    {
      close(rxfd);
      rxfd = -1;
    }

  if (txfd >= 0)
    {
      close(txfd);
      txfd = -1;
    }

  if (rxPath)
    {
      unlink(rxPath);
    }

  if (txPath)
    {
      unlink(txPath);
    }

  return OK;
}

int CProtoIpc::doStart()
{
  int ret;

  ret = startWorkerThread([this]() { thread(); });
  if (ret < 0)
    {
      DAWNERR("failed to start thread %d\n", ret);
      return ret;
    }

  DAWNINFO("IPC protocol started\n");

  return OK;
}

int CProtoIpc::doStop()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  cleanupNotifications();
#endif

  stopWorkerThread();

  DAWNINFO("IPC protocol stopped\n");

  return OK;
}

bool CProtoIpc::hasThread() const
{
  return workerThreadRunning();
}
