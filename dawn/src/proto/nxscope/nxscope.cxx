// dawn/src/proto/nxscope/nxscope.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/nxscope/nxscope.hxx"

#include <cstring>

#include "dawn/io/common.hxx"
#include "dawn/io/ddata.hxx"

using namespace dawn;

#if defined(CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD) + defined(CONFIG_DAWN_IO_NOTIFY) != 1
#  error one sampling method can be supported
#endif

// Interval for recv thread in us

#define NXSCOPE_RECV_INTERVAL (10000)

// Interval in us for sample thread

#define NXSCOPE_SAMPLE_INTERVAL (1000000)

static inline uint16_t nxscopeU16Le(const uint8_t *p)
{
  return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}

static inline uint32_t nxscopeU32Le(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CProtoNxscope::ioNotifierCb(void *priv, io_ddata_t *data)
{
  SProtoNxscopeIochan *chan = (SProtoNxscopeIochan *)priv;

  if (!chan || !chan->obj || !data || !chan->put)
    {
      return -EINVAL;
    }

  return chan->put(&chan->obj->nxs, chan->chan, data->getDataPtr(), data->getItems());
}
#endif

int CProtoNxscope::sendAck(int ret)
{
  size_t len;

  if (nxs.proto_cmd == nullptr || nxs.intf_cmd == nullptr)
    {
      return -EINVAL;
    }

  if (nxs.proto_cmd->ops == nullptr || nxs.intf_cmd->ops == nullptr)
    {
      return -EINVAL;
    }

  if (nxs.proto_cmd->ops->frame_final == nullptr || nxs.intf_cmd->ops->send == nullptr)
    {
      return -EINVAL;
    }

  if (nxs.txbuf == nullptr)
    {
      return -EINVAL;
    }

  if (nxs.txbuf_len < nxs.proto_cmd->hdrlen + sizeof(ret) + nxs.proto_cmd->footlen)
    {
      return -ENOBUFS;
    }

  len = nxs.proto_cmd->hdrlen;
  std::memcpy(&nxs.txbuf[len], &ret, sizeof(ret));
  len += sizeof(ret);

  ret = nxs.proto_cmd->ops->frame_final(nxs.proto_cmd, NXSCOPE_HDRID_ACK, nxs.txbuf, &len);
  if (ret < 0)
    {
      return ret;
    }

  return nxs.intf_cmd->ops->send(nxs.intf_cmd, nxs.txbuf, (int)len);
}

int CProtoNxscope::userIdCb(void *priv, uint8_t id, uint8_t *buff)
{
  CProtoNxscope *obj;
  int ret;
  bool handled;

  obj = static_cast<CProtoNxscope *>(priv);
  if (obj == nullptr || buff == nullptr)
    {
      return -EINVAL;
    }

  handled = (id == NXSCOPE_USER_SET_IO || id == NXSCOPE_USER_SET_IO_SEEK);
  if (!handled)
    {
      /* Ignore non-extension IDs routed through userid callback.
       * In particular, ACK frames may be seen here and must not be ACKed again.
       */
      return OK;
    }

  ret = obj->handleUserCommand(id, buff);

#ifdef CONFIG_LOGGING_NXSCOPE_ACKFRAMES
  {
    int ack;

    ack = obj->sendAck(ret);
    if (ack < 0)
      {
        DAWNERR("nxscope user ack failed: %d\n", ack);
        return ack;
      }
  }
#endif

  return ret;
}

int CProtoNxscope::handleUserCommand(uint8_t id, uint8_t *buff)
{
  switch (id)
    {
      case NXSCOPE_USER_SET_IO:
        {
          return userSetIO(buff);
        }

      case NXSCOPE_USER_SET_IO_SEEK:
        {
          return userSetIOSeek(buff);
        }

      default:
        {
          return -ENOTSUP;
        }
    }
}

CProtoNxscope::SProtoNxscopeIochan *CProtoNxscope::findIochan(SObjectId::ObjectId objid)
{
  for (auto &iochan : vio)
    {
      if (iochan.io != nullptr && iochan.io->getIdV() == objid)
        {
          return &iochan;
        }
    }

  return nullptr;
}

int CProtoNxscope::userSetIO(uint8_t *buff)
{
  SObjectId::ObjectId objid;
  uint16_t size;
  SProtoNxscopeIochan *iochan;
  CIOCommon *io;
  io_ddata_t *iodata;
  size_t oldSize;
  int ret;

  objid = nxscopeU32Le(buff);
  size = nxscopeU16Le(&buff[4]);

  iochan = findIochan(objid);
  if (iochan == nullptr || iochan->io == nullptr)
    {
      return -ENOENT;
    }

  io = iochan->io;
  if (!io->isWrite())
    {
      return -EPERM;
    }

  iodata = iochan->setData;
  if (iodata == nullptr)
    {
      return -ENOMEM;
    }

  if (!io->isSeekable())
    {
      if (size != io->getDataSize())
        {
          return -EINVAL;
        }

      std::memcpy(iodata->getDataPtr(), &buff[6], size);
      ret = io->setData(*iodata);
    }
  else
    {
      if (size > iodata->getDataSize())
        {
          return -EINVAL;
        }

      std::memcpy(iodata->getDataPtr(), &buff[6], size);

      oldSize = iodata->N;
      iodata->N = size;
      ret = io->setData(*iodata);
      iodata->N = oldSize;
    }

  return ret;
}

int CProtoNxscope::userSetIOSeek(uint8_t *buff)
{
  SObjectId::ObjectId objid;
  size_t offset;
  uint16_t size;
  SProtoNxscopeIochan *iochan;
  CIOCommon *io;
  io_ddata_t *iodata;
  size_t oldSize;
  int ret;

  objid = nxscopeU32Le(buff);
  offset = nxscopeU32Le(&buff[4]);
  size = nxscopeU16Le(&buff[8]);

  iochan = findIochan(objid);
  if (iochan == nullptr || iochan->io == nullptr)
    {
      return -ENOENT;
    }

  io = iochan->io;
  if (!io->isWrite())
    {
      return -EPERM;
    }

  if (!io->isSeekable())
    {
      return -ENOTSUP;
    }

  iodata = iochan->setData;
  if (iodata == nullptr)
    {
      return -ENOMEM;
    }

  if (size > iodata->getDataSize())
    {
      return -EINVAL;
    }

  std::memcpy(iodata->getDataPtr(), &buff[10], size);

  oldSize = iodata->N;
  iodata->N = size;
  ret = io->setData(*iodata, offset);
  iodata->N = oldSize;

  return ret;
}

int CProtoNxscope::configureNxscope()
{
  int ret;

  // Default serial protocol implementation

  ret = nxscope_proto_ser_init(&nxsProto, nullptr);
  if (ret != OK)
    {
      DAWNERR("nxscope_proto_ser_init failed: %d\n", ret);
      return ret;
    }

  // Connect callbacks

  nxsCbs.userid_priv = this;
  nxsCbs.userid = userIdCb;
  nxsCbs.start_priv = nullptr;
  nxsCbs.start = nullptr;

  // Initialize nxscope

  nxsCfg.streambuf_len = CONFIG_DAWN_PROTO_NXSCOPE_STREAMBUF_LEN;
  nxsCfg.rxbuf_len = CONFIG_DAWN_PROTO_NXSCOPE_RXBUF_LEN;
  nxsCfg.rx_padding = CONFIG_DAWN_PROTO_NXSCOPE_RX_PADDING;
#ifdef CONFIG_LOGGING_NXSCOPE_CRICHANNELS
  nxsCfg.cribuf_len = CONFIG_DAWN_PROTO_NXSCOPE_CRIBUF_LEN;
#endif

  nxsCfg.intf_cmd = &nxsIntf;
  nxsCfg.intf_stream = &nxsIntf;
  nxsCfg.proto_cmd = &nxsProto;
  nxsCfg.proto_stream = &nxsProto;
  nxsCfg.callbacks = &nxsCbs;
  nxsCfg.channels = vchannels.size();

  ret = nxscope_init(&nxs, &nxsCfg);
  if (ret != OK)
    {
      DAWNERR("nxscope_init failed: %d\n", ret);
      return ret;
    }

  return OK;
}

int CProtoNxscope::allocObject(SProtoNxscopeIOBind *alloc)
{
  DAWNINFO("allocate object 0x%" PRIx32 "\n", alloc->objid.v);

  // Allocate object in map

  setObjectMapItem(alloc->objid.v, nullptr);

  // Store reference to chanel configuration

  vchannels.push_back(alloc);

  return OK;
};

int CProtoNxscope::allocNames(size_t j, SProtoNxscopeNames *alloc)
{
  DAWNINFO("channel %zu with name %s\n", j, alloc->name);

  // Store reference to chanel configuration

  vnames.push_back(alloc);

  return OK;
};

uint8_t CProtoNxscope::getChannelDim(const CIOCommon &io)
{
  return io.getDataDim();
}

uint8_t CProtoNxscope::getChannelDtype(const CIOCommon &io)
{
  uint8_t dtype = io.getDtype();

  switch (dtype)
    {
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        {
          return NXSCOPE_TYPE_INT32;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        {
          return NXSCOPE_TYPE_UINT32;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT64
      case SObjectId::DTYPE_UINT64:
        {
          return NXSCOPE_TYPE_UINT64;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        {
          return NXSCOPE_TYPE_FLOAT;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_B16
      case SObjectId::DTYPE_B16:
        {
          return NXSCOPE_TYPE_B16;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UB16
      case SObjectId::DTYPE_UB16:
        {
          return NXSCOPE_TYPE_UB16;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_CHAR
      case SObjectId::DTYPE_CHAR:
        {
          return NXSCOPE_TYPE_CHAR;
        }
#endif

      default:
        {
          DAWNERR("unsuported nxscope type %d\n", dtype);
          return NXSCOPE_TYPE_NONE;
        }
    }
}

int CProtoNxscope::nxscopeChannelsCreate()
{
  union nxscope_chinfo_type_u u;
  const SProtoNxscopeIOBind *alloc;
  size_t bindidx;
  int chanid = 0;
  int ret;

  hasStreamChannels = false;

  for (bindidx = 0; bindidx < vchannels.size(); bindidx++)
    {
      alloc = vchannels[bindidx];
      const CIOCommon *io = getIO(alloc->objid.v);

      if (io != nullptr)
        {
          SProtoNxscopeIochan iochan;
          const char *name;

          if (bindidx < vnames.size())
            {
              name = vnames[bindidx]->name;
            }
          else
            {
              name = "no-name";
            }

          DAWNINFO("nxscope channel %s %p 0x%x\n", name, io, alloc->objid.v);

          // Store IO to nxscope chan map

          iochan.chan = chanid;
          iochan.dim = getChannelDim(*io);
          iochan.stream = io->isRead();
          iochan.io = (CIOCommon *)io;
          iochan.obj = this;
          iochan.setData = nullptr;
#ifdef CONFIG_DAWN_IO_NOTIFY
          iochan.put = nullptr;
#endif
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
          iochan.sample = nullptr;
          iochan.data = nullptr;
#endif

          if (io->isRead() == false && io->isWrite() == false)
            {
              DAWNERR("IO 0x%" PRIx32 " has neither read nor write support\n", alloc->objid.v);
              return -EPERM;
            }

          if (iochan.io->isWrite())
            {
              if (iochan.io->isSeekable())
                {
                  iochan.setData = iochan.io->ddata_alloc(1, CONFIG_DAWN_PROTO_NXSCOPE_RXBUF_LEN);
                }
              else
                {
                  iochan.setData = iochan.io->ddata_alloc(1);
                }

              if (iochan.setData == nullptr)
                {
                  DAWNERR("failed to allocate set buffer for objid=0x%" PRIx32 "\n",
                          alloc->objid.v);
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
                  delete iochan.data;
                  iochan.data = nullptr;
#endif
                  return -ENOMEM;
                }
            }

          if (iochan.stream)
            {
              ret = bindChannelCallbacks(iochan, io->getDtype());
              if (ret < 0)
                {
                  if (iochan.io->isWrite())
                    {
                      DAWNINFO("set-only nxscope channel for objid=0x%" PRIx32 "\n",
                               alloc->objid.v);
                      iochan.stream = false;
                    }
                  else
                    {
                      DAWNERR("Unsupported data type %d for objid=0x%" PRIx32 "\n",
                              io->getDtype(),
                              alloc->objid.v);
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
                      delete iochan.data;
                      iochan.data = nullptr;
#endif
                      delete iochan.setData;
                      iochan.setData = nullptr;
                      return ret;
                    }
                }
            }

#ifdef CONFIG_DAWN_IO_NOTIFY
          if (iochan.stream && iochan.io->isNotify() == false)
            {
              if (iochan.io->isWrite())
                {
                  DAWNINFO("set-only nxscope channel (no notify) for "
                           "objid=0x%" PRIx32 "\n",
                           alloc->objid.v);
                  iochan.stream = false;
                }
              else
                {
                  DAWNERR("notify not supported for objid=0x%" PRIx32 "\n", alloc->objid.v);
                  delete iochan.setData;
                  iochan.setData = nullptr;
                  return -ENOTSUP;
                }
            }
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
          if (iochan.stream && iochan.io->isSeekable())
            {
              if (iochan.io->isWrite())
                {
                  DAWNINFO("set-only nxscope channel (seekable) for "
                           "objid=0x%" PRIx32 "\n",
                           alloc->objid.v);
                  iochan.stream = false;
                }
              else
                {
                  DAWNERR("seekable IO not supported by NXScope "
                          "(objid=0x%08" PRIx32 ")\n",
                          iochan.io->getIdV());
                  delete iochan.setData;
                  iochan.setData = nullptr;
                  return -ENOTSUP;
                }
            }

          if (iochan.stream)
            {
              iochan.data = iochan.io->ddata_alloc(1);
              if (iochan.data == nullptr)
                {
                  DAWNERR("failed to allocate sample buffer for objid=0x%" PRIx32 "\n",
                          alloc->objid.v);
                  delete iochan.setData;
                  iochan.setData = nullptr;
                  return -ENOMEM;
                }
            }
#endif

          if (iochan.stream)
            {
              // Initialize nxscope stream channel
              u.s.dtype = getChannelDtype(*io);
              u.s._res = 0;
              u.s.cri = 0;
              // NOTE: nxscope_chan_init declares name as char* (non-const)
              // but never modifies it. The char* in the struct is only
              // stored and compared, so casting away const is safe here.
              nxscope_chan_init(&nxs, chanid, const_cast<char *>(name), u.u8, iochan.dim, 0);
              iochan.chan = chanid;
              chanid += 1;
              hasStreamChannels = true;
            }
          else
            {
              iochan.chan = 0;
            }

          vio.push_back(iochan);
        }
      else
        {
          DAWNERR("IO not found for objid=0x%" PRIx32 "\n", alloc->objid.v);
          return -EIO;
        }
    }

  return OK;
};

#ifdef CONFIG_DAWN_IO_NOTIFY
int CProtoNxscope::putInt32(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim)
{
  return nxscope_put_vint32(nxs, chan, static_cast<int32_t *>(data), dim);
}

int CProtoNxscope::putUint32(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim)
{
  return nxscope_put_vuint32(nxs, chan, static_cast<uint32_t *>(data), dim);
}

int CProtoNxscope::putUint64(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim)
{
  return nxscope_put_vuint64(nxs, chan, static_cast<uint64_t *>(data), dim);
}

int CProtoNxscope::putFloat(struct nxscope_s *nxs, uint8_t chan, void *data, uint8_t dim)
{
  return nxscope_put_vfloat(nxs, chan, static_cast<float *>(data), dim);
}
#endif

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
int CProtoNxscope::sampleInt32(CProtoNxscope *obj, SProtoNxscopeIochan *iochan)
{
  int ret;

  if (!iochan || !iochan->data)
    {
      return -EINVAL;
    }

  ret = iochan->io->getData(*iochan->data, 1);
  if (ret == OK)
    {
      ret = nxscope_put_vint32(
        &obj->nxs, iochan->chan, static_cast<int32_t *>(iochan->data->getDataPtr()), iochan->dim);
    }

  return ret;
}

int CProtoNxscope::sampleUint32(CProtoNxscope *obj, SProtoNxscopeIochan *iochan)
{
  int ret;

  if (!iochan || !iochan->data)
    {
      return -EINVAL;
    }

  ret = iochan->io->getData(*iochan->data, 1);
  if (ret == OK)
    {
      ret = nxscope_put_vuint32(
        &obj->nxs, iochan->chan, static_cast<uint32_t *>(iochan->data->getDataPtr()), iochan->dim);
    }

  return ret;
}

int CProtoNxscope::sampleUint64(CProtoNxscope *obj, SProtoNxscopeIochan *iochan)
{
  int ret;

  if (!iochan || !iochan->data)
    {
      return -EINVAL;
    }

  ret = iochan->io->getData(*iochan->data, 1);
  if (ret == OK)
    {
      ret = nxscope_put_vuint64(
        &obj->nxs, iochan->chan, static_cast<uint64_t *>(iochan->data->getDataPtr()), iochan->dim);
    }

  return ret;
}

int CProtoNxscope::sampleFloat(CProtoNxscope *obj, SProtoNxscopeIochan *iochan)
{
  int ret;

  if (!iochan || !iochan->data)
    {
      return -EINVAL;
    }

  ret = iochan->io->getData(*iochan->data, 1);
  if (ret == OK)
    {
      ret = nxscope_put_vfloat(
        &obj->nxs, iochan->chan, static_cast<float *>(iochan->data->getDataPtr()), iochan->dim);
    }

  return ret;
}
#endif

int CProtoNxscope::bindChannelCallbacks(SProtoNxscopeIochan &iochan, uint8_t dtype)
{
  switch (dtype)
    {
#ifdef CONFIG_DAWN_DTYPE_INT32
      case SObjectId::DTYPE_INT32:
        {
#  ifdef CONFIG_DAWN_IO_NOTIFY
          iochan.put = &putInt32;
#  endif
#  ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
          iochan.sample = &sampleInt32;
#  endif
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT32
      case SObjectId::DTYPE_UINT32:
        {
#  ifdef CONFIG_DAWN_IO_NOTIFY
          iochan.put = &putUint32;
#  endif
#  ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
          iochan.sample = &sampleUint32;
#  endif
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_UINT64
      case SObjectId::DTYPE_UINT64:
        {
#  ifdef CONFIG_DAWN_IO_NOTIFY
          iochan.put = &putUint64;
#  endif
#  ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
          iochan.sample = &sampleUint64;
#  endif
          break;
        }
#endif

#ifdef CONFIG_DAWN_DTYPE_FLOAT
      case SObjectId::DTYPE_FLOAT:
        {
#  ifdef CONFIG_DAWN_IO_NOTIFY
          iochan.put = &putFloat;
#  endif
#  ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
          iochan.sample = &sampleFloat;
#  endif
          break;
        }
#endif

      default:
        {
          return -ENOTSUP;
        }
    }

  return OK;
}

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
void CProtoNxscope::getAndPut(SProtoNxscopeIochan *iochan)
{
  int ret;

  if (!iochan || !iochan->sample)
    {
      return;
    }

  ret = iochan->sample(this, iochan);
  if (ret < 0 && ret != -EAGAIN)
    {
      DAWNERR("sampling failed for chan=%d ret=%d\n", iochan->chan, ret);
    }
}
#endif

void CProtoNxscope::threadRecv()
{
  // Loop until stop called

  do
    {
      int ret;

      /* Flush stream data */

      if (hasStreamChannels)
        {
          ret = nxscope_stream(&nxs);
          if (ret < 0)
            {
              DAWNERR("ERROR: nxscope_stream failed %d\n", ret);
            }
        }

      /* Handle recv data */

      ret = nxscope_recv(&nxs);
      if (ret < 0)
        {
          DAWNERR("ERROR: nxscope_recv failed %d\n", ret);
        }

#if NXSCOPE_RECV_NONBLOCK == 1
      // Sleep if read is non-blocking

      usleep(NXSCOPE_RECV_INTERVAL);
#endif
    }
  while (!threadRecvMember.shouldQuit());

  // Mark thread quit done

  threadRecvMember.markThreadFinished();
}

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
void CProtoNxscope::threadSample()
{
  // Loop until stop called

  do
    {
      for (auto &io : vio)
        {
          if (!io.stream)
            {
              continue;
            }
          getAndPut(&io);
        }

      usleep(NXSCOPE_SAMPLE_INTERVAL);
    }
  while (!threadSampleMember.shouldQuit());

  // Mark thread quit done

  threadSampleMember.markThreadFinished();
}

#endif // CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD

CProtoNxscope::~CProtoNxscope()
{
  for (auto &io : vio)
    {
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
      delete io.data;
      io.data = nullptr;
#endif
      delete io.setData;
      io.setData = nullptr;
    }

  vchannels.clear();
  vnames.clear();
  vio.clear();

  // Deinit nxscope

  nxscope_deinit(&nxs);
}

int CProtoNxscope::configure()
{
  int ret;

  ret = initPriv();
  if (ret != OK)
    {
      return ret;
    }

  return OK;
}

int CProtoNxscope::init()
{
  // Allocated objects are mapped after handler bind, so create channels here
  return nxscopeChannelsCreate();
}

int CProtoNxscope::deinit()
{
  int ret;

  ret = deinitPriv();
  if (ret != OK)
    {
      return ret;
    }

  return OK;
}

int CProtoNxscope::doStart()
{
  int ret;

  ret = startPriv();
  if (ret != OK)
    {
      return ret;
    }

  // Start recv thread

  threadRecvMember.setThreadFunc([this]() { threadRecv(); });
  ret = threadRecvMember.threadStart();
  if (ret < 0)
    {
      return ret;
    }

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
  // Start sample thread

  threadSampleMember.setThreadFunc([this]() { threadSample(); });
  ret = threadSampleMember.threadStart();
  if (ret < 0)
    {
      return ret;
    }

#else
  // Register notifications

  for (auto &io : vio)
    {
      if (!io.stream)
        {
          continue;
        }
      ret = io.io->setNotifier(ioNotifierCb, 0, (void *)&io);
      if (ret < 0)
        {
          if (io.io->isWrite())
            {
              DAWNINFO("set notifier failed for writable objId = 0x%" PRIx32
                       ", fallback to set-only (%d)\n",
                       io.io->getIdV(),
                       ret);
              io.stream = false;
              continue;
            }

          DAWNERR("set notifier failed for objId = 0x%" PRIx32 "\n", io.io->getIdV());
          return ret;
        }
    }
#endif

  return OK;
};

int CProtoNxscope::doStop()
{
  int ret;

  // Stop transport first to unblock recv path before thread joins

  ret = stopPriv();

  // Stop recv thread

  threadRecvMember.threadStop();

#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
  // Stop sample thread

  threadSampleMember.threadStop();
#endif

  return ret;
};

bool CProtoNxscope::hasThread() const
{
#ifdef CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD
  return !(threadRecvMember.isStopped() & threadSampleMember.isStopped());
#else
  return !threadRecvMember.isStopped();
#endif
}
