// dawn/src/proto/serial/simple.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/serial/simple.hxx"

#include "dawn/common/poll_loop.hxx"

#include <fcntl.h>
#include <poll.h>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>

#include <cstring>

using namespace dawn;

int CProtoSerial::sendFrame(uint8_t cmd, const uint8_t *payload, size_t len)
{
  uint8_t frame[FRAME_MAX_PAYLOAD + FRAME_MIN_LEN];
  size_t tosend;
  ssize_t ret;
  uint16_t crc;

  if (len > FRAME_MAX_PAYLOAD || fd < 0)
    {
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

  ret = write(fd, frame, tosend);
  if (ret < 0 || static_cast<size_t>(ret) != tosend)
    {
      DAWNERR(
        "Serial sendto failed cmd=0x%02x ret=%zd exp=%zu errno=%d\n", cmd, ret, tosend, errno);
      return ret < 0 ? static_cast<int>(ret) : -EIO;
    }

  return OK;
}

void CProtoSerial::thread()
{
  struct pollfd fds[1];
  SPollLoopCallbacks callbacks;

  std::memset(fds, 0, sizeof(fds));

  fds[0].fd = fd;
  fds[0].events = POLLIN;
  parserPos = 0;
  parserLen = 0;
  parserState = 0;

  callbacks.beforePoll = CProtoSerial::cbPollBefore;
  callbacks.afterPoll = CProtoSerial::cbPollAfter;
  callbacks.onPollReady = CProtoSerial::cbPollOnReady;

  CPollLoopRunner::run(workerThread(), fds, 1, DAWN_POLL_TIMEOUT_MS, callbacks, this);
}

int CProtoSerial::pollBefore(struct pollfd *pfds, nfds_t nfds)
{
  if (pfds == nullptr || nfds == 0)
    {
      return -EINVAL;
    }

  pfds[0].revents = 0;
  return OK;
}

void CProtoSerial::pollAfter(int ret)
{
  if (ret < 0)
    {
      DAWNERR("serial poll failed %d\n", -errno);
    }
}

int CProtoSerial::pollOnReady(struct pollfd *pfds, nfds_t nfds, int pollRet)
{
  uint8_t byte;
  ssize_t ret;

  if (pfds == nullptr || nfds == 0 || pollRet <= 0)
    {
      return -EINVAL;
    }

  if ((pfds[0].revents & POLLIN) == 0)
    {
      return OK;
    }

  ret = read(fd, &byte, 1);
  if (ret <= 0)
    {
      usleep(10000);
      return static_cast<int>(ret);
    }

  switch (parserState)
    {
      case 0:
        {
          if (byte == FRAME_SYNC)
            {
              rxbuffer[parserPos] = byte;
              parserPos = 1;
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
              return OK;
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
              handleFrame(rxbuffer, parserPos);
              parserState = 0;
              parserPos = 0;
            }

          break;
        }
    }

  pfds[0].revents = 0;
  return OK;
}

int CProtoSerial::cbPollBefore(void *priv, struct pollfd *pfds, nfds_t nfds)
{
  CProtoSerial *self;

  self = static_cast<CProtoSerial *>(priv);
  if (self == nullptr)
    {
      return -EINVAL;
    }

  return self->pollBefore(pfds, nfds);
}

void CProtoSerial::cbPollAfter(void *priv, struct pollfd *pfds, nfds_t nfds, int ret)
{
  CProtoSerial *self;

  (void)pfds;
  (void)nfds;
  self = static_cast<CProtoSerial *>(priv);
  if (self == nullptr)
    {
      return;
    }

  self->pollAfter(ret);
}

int CProtoSerial::cbPollOnReady(void *priv, struct pollfd *pfds, nfds_t nfds, int pollRet)
{
  CProtoSerial *self;

  self = static_cast<CProtoSerial *>(priv);
  if (self == nullptr)
    {
      return -EINVAL;
    }

  return self->pollOnReady(pfds, nfds, pollRet);
}

int CProtoSerial::configureDesc(const CDescObject &desc)
{
  SObjectCfg::SObjectCfgItem *item = nullptr;
  size_t offset;

  offset = 0;

  for (size_t i = 0; i < desc.getSize(); i++)
    {
      item = desc.objectCfgItemNext(offset);

      if (item->cfgid.s.cls != CProtoCommon::PROTO_CLASS_SERIAL)
        {
          DAWNERR("Unsupported SERIAL config 0x%08" PRIx32 "\n", item->cfgid.v);
          return -EINVAL;
        }

      switch (item->cfgid.s.id)
        {
          case PROTO_SERIAL_CFG_IOBIND:
            {
              for (size_t j = 0; j < item->cfgid.s.size;)
                {
                  SProtoSerialIOBind *tmp;

                  tmp = reinterpret_cast<SProtoSerialIOBind *>(item->data + j);

                  allocObject(tmp);
                  j += sizeof(SProtoSerialIOBind) / 4;
                }

              break;
            }

          case PROTO_SERIAL_CFG_PATH:
            {
              path = reinterpret_cast<const char *>(&item->data);
              break;
            }

          case PROTO_SERIAL_CFG_BAUD:
            {
              baud = static_cast<uint32_t>(item->data[0]);
              break;
            }

          default:
            {
              DAWNERR("Unsupported SERIAL config 0x%08" PRIx32 "\n", item->cfgid.v);
              return -EINVAL;
            }
        }
    }

  return OK;
}

int CProtoSerial::serialInit()
{
  struct termios tio;

  if (!path)
    {
      DAWNERR("Serial path not configured\n");
      return -1;
    }

  fd = open(path, O_RDWR);
  if (fd < 0)
    {
      DAWNERR("Failed to open serial port: %s\n", path);
      return -1;
    }

  tcgetattr(fd, &tio);
  cfmakeraw(&tio);
  tcsetattr(fd, TCSANOW, &tio);

  DAWNINFO("Serial port initialized: %s (baud=%u)\n", path, baud);

  return 0;
}

CProtoSerial::~CProtoSerial()
{
  deinit();
}

int CProtoSerial::configure()
{
  int ret;

  ret = configureDesc(getDesc());
  if (ret != OK)
    {
      DAWNERR("Serial configure failed (error %d)\n", ret);
      return ret;
    }

  return OK;
}

int CProtoSerial::init()
{
  int ret;

  ret = serialInit();
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

int CProtoSerial::deinit()
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

int CProtoSerial::doStart()
{
  int ret;

  ret = startWorkerThread([this]() { thread(); });
  if (ret < 0)
    {
      DAWNERR("failed to start thread %d\n", ret);
      return ret;
    }

  DAWNINFO("Serial protocol started\n");

  return 0;
}

int CProtoSerial::doStop()
{
#ifdef CONFIG_DAWN_IO_NOTIFY
  cleanupNotifications();
#endif

  stopWorkerThread();

  DAWNINFO("Serial protocol stopped\n");

  return 0;
}

bool CProtoSerial::hasThread() const
{
  return workerThreadRunning();
}
