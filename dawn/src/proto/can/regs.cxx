// dawn/src/proto/can/regs.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "can_internal.hxx"

#include <new>

#include "dawn/io/common.hxx"
#include "dawn/porting/can.hxx"

using namespace dawn;

static constexpr size_t CAN_SEGMENT_TRANSFER_MAX = 0x380;
static constexpr size_t CAN_SINGLE_ID_TRANSFER_MAX = 0x300;

static size_t canIoDataBytes(CIOCommon *io)
{
  return io->getDataSize();
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CProtoCan::notifierCb(void *priv, io_ddata_t *data)
{
  SCanIoData *canio = (SCanIoData *)priv;
  CIOCommon *io;
  dawn::porting::canmsg_s msg;

  if (canio == nullptr)
    {
      DAWNERR("NULL canio pointer in notifier callback\n");
      return -EINVAL;
    }

  io = canio->io;
  if (io == nullptr)
    {
      DAWNERR("NULL io pointer in notifier callback\n");
      return -EINVAL;
    }

  msg.id = canio->canid;
  msg.len = io->getDataSize();
  msg.rtr = 0;
  msg.extid = CAN_EXTID_FLAG;
  msg._res = 0;

  std::memcpy(msg.data, data->getDataPtr(), io->getDataSize());

  return can_send(canio->proto->fd, &msg);
}
#endif

int CProtoCan::validateCanIo(const SProtoCanIOBind *v, CIOCommon *io, SCanIoData *canio)
{
  switch (v->type)
    {
#ifdef CONFIG_DAWN_PROTO_CAN_SIMPLE
#  ifdef CONFIG_DAWN_IO_NOTIFY
      case CAN_TYPE_PUSH:
        {
          if (io->isRead() == false)
            {
              DAWNERR("input reg doesn't support read\n");
              return -EINVAL;
            }

          if (io->isNotify() == true)
            {
              int ret = io->setNotifier(notifierCb, 0, canio);
              if (ret < 0)
                {
                  DAWNERR("ERROR: set notifier failed for objId = "
                          "0x%" PRIx32 "\n",
                          io->getIdV());
                }
            }
          else
            {
              DAWNERR("IO notify required\n");
              return -EINVAL;
            }

          if (canio->data_len > 8)
            {
              DAWNERR("not supported\n");
              return -EINVAL;
            }

          break;
        }
#  endif

      case CAN_TYPE_READ:
        {
#  ifdef CONFIG_DAWN_PROTO_CAN_RTR
          if (io->isRead() == false)
            {
              DAWNERR("input reg doesn't support read\n");
              return -EINVAL;
            }

          if (canio->data_len > 8)
            {
              DAWNERR("not supported\n");
              return -EINVAL;
            }
#  else
          DAWNERR("CAN RTR required\n");
          return -EINVAL;
#  endif
          break;
        }

      case CAN_TYPE_WRITE:
        {
          if (io->isWrite() == false)
            {
              DAWNERR("reg doesn't support write\n");
              return -EINVAL;
            }

          if (canio->data_len > 8)
            {
              DAWNERR("not supported\n");
              return -EINVAL;
            }

          break;
        }
#endif

#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
      case CAN_TYPE_INDEXED_READ:
        {
          if (io->isRead() == false)
            {
              DAWNERR("input reg doesn't support read\n");
              return -EINVAL;
            }

          break;
        }

      case CAN_TYPE_INDEXED_WRITE:
        {
          if (io->isWrite() == false)
            {
              DAWNERR("reg doesn't support write\n");
              return -EINVAL;
            }

          break;
        }
#endif

#ifdef CONFIG_DAWN_PROTO_CAN_SEG
      case CAN_TYPE_READ_SEG:
        {
          if (io->isRead() == false)
            {
              DAWNERR("input reg doesn't support read\n");
              return -EINVAL;
            }

          if (!io->isSeekable() && canio->data_len > CAN_SEGMENT_TRANSFER_MAX)
            {
              DAWNERR("not supported\n");
              return -EINVAL;
            }
          break;
        }

      case CAN_TYPE_WRITE_SEG:
        {
          if (io->isWrite() == false)
            {
              DAWNERR("reg doesn't support write\n");
              return -EINVAL;
            }

          if (canio->data_len > CAN_SEGMENT_TRANSFER_MAX)
            {
              DAWNERR("not supported\n");
              return -EINVAL;
            }

          break;
        }
#endif

      default:
        {
          DAWNERR("unsupported CAN access type\n");
          return -EINVAL;
        }
    }

  return OK;
}

int CProtoCan::createRegsForGroup(SProtoCanIOBind *v)
{
  SProtoCanRegs *tmp;
  int ret;

  tmp = new (std::nothrow) SProtoCanRegs();
  if (!tmp)
    {
      DAWNERR("failed to allocate register handler\n");
      return -ENOMEM;
    }

  vregs.push_back(tmp);

  tmp->cfg = v;
  tmp->ios = 0;

#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
  if (v->type == CAN_TYPE_INDEXED_WRITE)
    {
      tmp->write_all = new (std::nothrow) SProtoCanWriteAll();
      if (!tmp->write_all)
        {
          return -ENOMEM;
        }

      CIsoTp::initState(tmp->write_all->isotp);
      tmp->write_all->io_idx = 0;
      tmp->write_all->io_off = 0;
    }
  else
    {
      tmp->write_all = nullptr;
    }
#endif

  tmp->io = new (std::nothrow) SCanIoData[v->size]();
  if (!tmp->io)
    {
      return -ENOMEM;
    }

#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
  size_t group_total = 0;
#endif

  for (size_t i = 0; i < v->size; i++)
    {
      CIOCommon *io;
      io_ddata_t *iobuffer;
      SCanIoData *canio = &tmp->io[i];

      canio->proto = this;

      io = getIO(v->objid[i]);
      if (io == nullptr)
        {
          DAWNERR("Failed to get IO 0x%08" PRIx32 "\n", v->objid[i]);
          return -EIO;
        }

      if (io->isSeekable() && v->type != CAN_TYPE_READ_SEG && v->type != CAN_TYPE_WRITE_SEG)
        {
          DAWNERR("seekable IO is supported only for segmented read "
                  "or write types (objid=0x%08" PRIx32 ", type=%" PRIu32 ")\n",
                  io->getIdV(),
                  v->type);
          return -ENOTSUP;
        }

      size_t chunk_size = 0;

      if (io->isSeekable())
        {
          if (v->type == CAN_TYPE_READ_SEG)
            {
              chunk_size = 64;
            }
          else if (v->type == CAN_TYPE_WRITE_SEG)
            {
              chunk_size = CAN_SEGMENT_TRANSFER_MAX;
            }
        }

      iobuffer = io->ddata_alloc(1, chunk_size);
      if (!iobuffer)
        {
          DAWNERR("failed to allocate register buffer\n");
          return -ENOMEM;
        }

      canio->iodata = iobuffer;
      canio->io = io;
      canio->data_len = canIoDataBytes(io);
      if (io->isSeekable() && v->type == CAN_TYPE_WRITE_SEG)
        {
          canio->data_len = iobuffer->getDataSize();
        }

#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
      if (v->type == CAN_TYPE_INDEXED_READ || v->type == CAN_TYPE_INDEXED_WRITE)
        {
          canio->canid = nodeid + v->start;
        }
      else
#endif
        {
          canio->canid = nodeid + v->start + tmp->ios;
        }

#ifdef DAWN_PROTO_HAS_ISOTP
      if (v->type == CAN_TYPE_WRITE_SEG || v->type == CAN_TYPE_INDEXED_WRITE)
        {
          canio->isotp = new (std::nothrow) CIsoTp::State();
          if (!canio->isotp)
            {
              return -ENOMEM;
            }

          CIsoTp::initState(*canio->isotp);
        }
      else
        {
          canio->isotp = nullptr;
        }
#endif

#ifdef CONFIG_DAWN_PROTO_CAN_EXTID
      if (canio->canid > CAN_MAX_EXTMSGID)
        {
          DAWNERR("CAN ID 0x%08" PRIx32 " exceeds extended ID max\n",
                  static_cast<uint32_t>(canio->canid));
          return -EINVAL;
        }
#else
      if (canio->canid > CAN_MAX_STDMSGID)
        {
          DAWNERR("CAN ID 0x%08" PRIx32 " exceeds standard ID max\n",
                  static_cast<uint32_t>(canio->canid));
          return -EINVAL;
        }
#endif

      ret = validateCanIo(v, io, canio);
      if (ret < 0)
        {
          return ret;
        }

#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
      if (v->type == CAN_TYPE_INDEXED_READ || v->type == CAN_TYPE_INDEXED_WRITE)
        {
          if (v->size > 0xff)
            {
              DAWNERR("too many IOs for single ID\n");
              return -EINVAL;
            }

          if (canio->data_len > CAN_SINGLE_ID_TRANSFER_MAX)
            {
              DAWNERR("not supported\n");
              return -EINVAL;
            }

          group_total += canio->data_len;
          if (group_total > CAN_SINGLE_ID_TRANSFER_MAX)
            {
              DAWNERR("not supported\n");
              return -EINVAL;
            }
        }
#endif

      tmp->ios += 1;
    }

  return OK;
}

int CProtoCan::createRegs()
{
  int ret;

  if (initialized)
    {
      return OK;
    }

  for (auto *v : valloc)
    {
      ret = createRegsForGroup(v);
      if (ret < 0)
        {
          destroyRegs();
          return ret;
        }
    }

  initialized = true;
  return OK;
}

int CProtoCan::destroyRegs()
{
  for (auto *v : vregs)
    {
      if (v == nullptr)
        {
          continue;
        }

      if (v->io)
        {
          for (size_t i = 0; i < v->ios; i++)
            {
              SCanIoData *canio = &v->io[i];

#ifdef CONFIG_DAWN_IO_NOTIFY
              if (canio->io && canio->io->isNotify())
                {
                  canio->io->setNotifier(nullptr, 0, nullptr);
                }
#endif

              if (canio->iodata)
                {
                  delete canio->iodata;
                  canio->iodata = nullptr;
                }

#ifdef DAWN_PROTO_HAS_ISOTP
              if (canio->isotp)
                {
                  delete canio->isotp;
                  canio->isotp = nullptr;
                }
#endif
            }

          delete[] v->io;
          v->io = nullptr;
        }

#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
      if (v->write_all)
        {
          delete v->write_all;
          v->write_all = nullptr;
        }
#endif

      delete v;
    }

  valloc.clear();
  vregs.clear();
  initialized = false;

  return OK;
}

int CProtoCan::msgRecv(const dawn::porting::canmsg_s &msg)
{
  int ret;

  for (auto v : vregs)
    {
#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
      if (v->cfg->type == CAN_TYPE_INDEXED_READ || v->cfg->type == CAN_TYPE_INDEXED_WRITE)
        {
          ret = handleSingleIdGroup(msg, v);
        }
      else
#endif
        {
          ret = handleLegacyGroup(msg, v);
        }

      if (ret != -ENOMSG)
        {
          return ret;
        }
    }

  return OK;
}

#ifdef DAWN_PROTO_HAS_ISOTP
int CProtoCan::sendSegmented(uint16_t canid,
                             const uint8_t *data,
                             size_t len,
                             bool with_index,
                             uint8_t index)
{
  int ret;
  size_t offset = 0;
  uint8_t seg = 0;
  size_t header = with_index ? 2 : 1;
  size_t payload_max = with_index ? 6 : 7;

  if (len > payload_max * 0x80)
    {
      return -EINVAL;
    }

  if (len == 0)
    {
      dawn::porting::canmsg_s resp;

      resp.id = canid;
      resp.len = header;
      resp.rtr = 0;
      resp.extid = CAN_EXTID_FLAG;
      resp._res = 0;
      resp.data[0] = 0x80;
      if (with_index)
        {
          resp.data[1] = index;
        }

      return can_send(fd, &resp);
    }

  while (offset < len)
    {
      dawn::porting::canmsg_s resp;
      size_t chunk = len - offset;
      bool last = false;

      if (chunk > payload_max)
        {
          chunk = payload_max;
        }
      else
        {
          last = true;
        }

      resp.id = canid;
      resp.len = header + chunk;
      resp.rtr = 0;
      resp.extid = CAN_EXTID_FLAG;
      resp._res = 0;

      resp.data[0] = seg;
      if (last)
        {
          resp.data[0] |= 0x80;
        }

      if (with_index)
        {
          resp.data[1] = index;
          std::memcpy(resp.data + 2, data + offset, chunk);
        }
      else
        {
          std::memcpy(resp.data + 1, data + offset, chunk);
        }

      ret = can_send(fd, &resp);
      if (ret < 0)
        {
          return ret;
        }

      offset += chunk;
      seg += 1;
    }

  return OK;
}

int CProtoCan::sendSegmentedRead(SCanIoData *canio,
                                 size_t len,
                                 bool with_index,
                                 uint8_t index,
                                 size_t offset)
{
  int ret;
  CIOCommon *io;
  io_ddata_t *iodata;
  size_t total_len;
  size_t payload_max = with_index ? 6 : 7;
  size_t max_window = payload_max * CAN_SEG_SEQ_MAX;

  if (!canio || !canio->io || !canio->iodata)
    {
      return -EINVAL;
    }

  io = canio->io;
  iodata = canio->iodata;
  total_len = io->isSeekable() ? io->getDataSize() : canio->data_len;

  if (!io->isSeekable())
    {
      return sendSegmented(
        canio->canid, static_cast<const uint8_t *>(iodata->getDataPtr()), len, with_index, index);
    }

  if (offset >= total_len)
    {
      return -EINVAL;
    }

  if (offset + len > total_len)
    {
      len = total_len - offset;
    }

  if (len > max_window)
    {
      len = max_window;
    }

  size_t sent = 0;
  uint8_t seg = 0;
  size_t buf_off = 0;
  size_t buf_len = 0;

  while (sent < len)
    {
      dawn::porting::canmsg_s resp;

      if (buf_off >= buf_len)
        {
          // Buffer empty, read more

          buf_len = len - sent;
          if (buf_len > iodata->getDataSize())
            {
              buf_len = iodata->getDataSize();
            }

          ret = io->getData(*iodata, 1, offset + sent);
          if (ret < 0)
            {
              return ret;
            }

          buf_off = 0;
        }

      size_t remaining = buf_len - buf_off;
      size_t chunk = remaining;

      if (chunk > payload_max)
        {
          chunk = payload_max;
        }

      resp.id = canio->canid;
      resp.len = (with_index ? 2 : 1) + chunk;
      resp.rtr = 0;
      resp.extid = CAN_EXTID_FLAG;
      resp._res = 0;
      resp.data[0] = seg;

      if ((sent + chunk) >= len)
        {
          resp.data[0] |= 0x80;
        }

      if (with_index)
        {
          resp.data[1] = index;
          std::memcpy(resp.data + 2, (uint8_t *)iodata->getDataPtr() + buf_off, chunk);
        }
      else
        {
          std::memcpy(resp.data + 1, (uint8_t *)iodata->getDataPtr() + buf_off, chunk);
        }

      ret = can_send(fd, &resp);
      if (ret < 0)
        {
          return ret;
        }

      sent += chunk;
      buf_off += chunk;
      seg += 1;
    }

  return OK;
}
#endif

int CProtoCan::sendIoResponse(SCanIoData *canio, uint8_t index, bool segmented)
{
  int ret;
  size_t data_len;

  data_len = canio->data_len;
  if (canio->io->isSeekable())
    {
      data_len = canio->io->getDataSize();
    }

  if (!segmented || !canio->io->isSeekable())
    {
      ret = canio->io->getData(*canio->iodata, 1);
      if (ret < 0)
        {
          DAWNERR("failed to get IO\n");
          return ret;
        }
    }

  if (!segmented)
    {
      dawn::porting::canmsg_s resp;

      if (data_len > 8)
        {
          return -ENOSYS;
        }

      resp.id = canio->canid;
      resp.len = data_len;
      resp.rtr = 0;
      resp.extid = CAN_EXTID_FLAG;
      resp._res = 0;

      std::memcpy(resp.data, canio->iodata->getDataPtr(), resp.len);

      return can_send(fd, &resp);
    }

#ifdef DAWN_PROTO_HAS_ISOTP
  return sendSegmentedRead(canio, data_len, index > 0, index);
#else
  return -ENOTSUP;
#endif
}

#ifdef CONFIG_DAWN_PROTO_CAN_SINGLE_ID
int CProtoCan::handleSingleIdReadAll(const dawn::porting::canmsg_s &msg,
                                     SProtoCanRegs *v,
                                     bool ignore_len)
{
  int ret;

  if (!ignore_len && msg.len != 2)
    {
      return OK;
    }

  for (size_t i = 0; i < v->ios; i++)
    {
      SCanIoData *canio = &v->io[i];

      ret = sendIoResponse(canio, static_cast<uint8_t>(i + 1), true);
      if (ret < 0)
        {
          return ret;
        }
    }

  return OK;
}

int CProtoCan::handleSingleIdWriteAll(const dawn::porting::canmsg_s &msg,
                                      SProtoCanRegs *v,
                                      uint8_t seg_no,
                                      bool seg_last)
{
  int ret;
  size_t payload_len = msg.len - 2;
  size_t payload_off = 0;
  uint64_t now = canTimestampUs();
  uint64_t timeout = CONFIG_DAWN_PROTO_CAN_SEG_TIMEOUT_US;

  if (!v->write_all->isotp.active)
    {
      if (seg_no != 0)
        {
          return -EINVAL;
        }

      v->write_all->isotp.active = true;
      v->write_all->isotp.seq_next = 0;
      v->write_all->io_idx = 0;
      v->write_all->io_off = 0;
      v->write_all->isotp.timestamp = now;
    }
  else if (timeout > 0 && (now - v->write_all->isotp.timestamp) > timeout)
    {
      v->write_all->isotp.active = false;
      if (seg_no != 0)
        {
          return -ETIMEDOUT;
        }

      v->write_all->isotp.active = true;
      v->write_all->isotp.seq_next = 0;
      v->write_all->io_idx = 0;
      v->write_all->io_off = 0;
      v->write_all->isotp.timestamp = now;
    }

  if (seg_no != v->write_all->isotp.seq_next)
    {
      v->write_all->isotp.active = false;
      return -EINVAL;
    }

  while (payload_off < payload_len && v->write_all->io_idx < v->ios)
    {
      SCanIoData *canio = &v->io[v->write_all->io_idx];
      size_t need = canio->data_len - v->write_all->io_off;
      size_t avail = payload_len - payload_off;
      size_t chunk = (avail < need) ? avail : need;

      std::memcpy((uint8_t *)canio->iodata->getDataPtr() + v->write_all->io_off,
                  msg.data + 2 + payload_off,
                  chunk);

      v->write_all->io_off += chunk;
      payload_off += chunk;

      if (v->write_all->io_off == canio->data_len)
        {
          ret = canio->io->setData(*canio->iodata);
          if (ret != OK)
            {
              DAWNERR("failed to set IO\n");
              v->write_all->isotp.active = false;
              return ret;
            }

          v->write_all->io_idx += 1;
          v->write_all->io_off = 0;
        }
    }

  if (payload_off != payload_len)
    {
      v->write_all->isotp.active = false;
      return -EINVAL;
    }

  if (seg_last)
    {
      if (v->write_all->io_idx != v->ios)
        {
          v->write_all->isotp.active = false;
          return -EINVAL;
        }

      CIsoTp::resetState(v->write_all->isotp);
      v->write_all->io_idx = 0;
      v->write_all->io_off = 0;

      return OK;
    }

  v->write_all->isotp.timestamp = now;
  v->write_all->isotp.seq_next += 1;
  return OK;
}

int CProtoCan::handleSingleIdIndex(const dawn::porting::canmsg_s &msg,
                                   SProtoCanRegs *v,
                                   uint8_t index,
                                   uint8_t seg_no,
                                   bool seg_last)
{
  int ret;
  SCanIoData *canio = &v->io[index - 1];

  if (v->cfg->type == CAN_TYPE_INDEXED_WRITE)
    {
      uint64_t now = canTimestampUs();
      uint64_t timeout = CONFIG_DAWN_PROTO_CAN_SEG_TIMEOUT_US;

      ret = CIsoTp::receiveCustom(msg,
                                  *canio->isotp,
                                  canio->iodata->getDataPtr(),
                                  canio->data_len,
                                  seg_no,
                                  seg_last,
                                  now,
                                  timeout);
      if (ret < 0)
        {
          return ret;
        }

      if (ret == 1) // Complete
        {
          ret = canio->io->setData(*canio->iodata);
          if (ret != OK)
            {
              DAWNERR("failed to set IO\n");
              return ret;
            }
        }

      return OK;
    }

  if (msg.len != 2)
    {
      return OK;
    }

  if (canio->io->isRead() == false)
    {
      DAWNERR("IO doesn't support read\n");
      return -EINVAL;
    }

  return sendIoResponse(canio, index, true);
}

int CProtoCan::handleSingleIdGroup(const dawn::porting::canmsg_s &msg, SProtoCanRegs *v)
{
  if (v->cfg->type != CAN_TYPE_INDEXED_READ && v->cfg->type != CAN_TYPE_INDEXED_WRITE)
    {
      return -ENOMSG;
    }

  if (msg.id != nodeid + v->cfg->start)
    {
      return -ENOMSG;
    }

  if (msg.len < 2)
    {
      return OK;
    }

  uint8_t seg = msg.data[0];
  uint8_t seg_no = seg & 0x7f;
  bool seg_last = (seg & 0x80) != 0;
  uint8_t index = msg.data[1];

  if (seg_no != 0 && v->cfg->type == CAN_TYPE_INDEXED_READ)
    {
      return -ENOMSG;
    }

  if (v->cfg->type == CAN_TYPE_INDEXED_READ && msg.len != 2)
    {
      // Ignore loopback of our own indexed read response frames. They share
      // the same CAN ID but are not request frames.
      return -ENOMSG;
    }

  if (index == 0)
    {
      if (v->cfg->type == CAN_TYPE_INDEXED_READ)
        {
          return handleSingleIdReadAll(msg, v, true);
        }

      return handleSingleIdWriteAll(msg, v, seg_no, seg_last);
    }

  if (index > v->cfg->size)
    {
      return OK;
    }

  return handleSingleIdIndex(msg, v, index, seg_no, seg_last);
}
#endif

int CProtoCan::handleLegacyGroup(const dawn::porting::canmsg_s &msg, SProtoCanRegs *v)
{
  int ret;

  if (msg.id < nodeid + v->cfg->start || msg.id >= nodeid + v->cfg->start + v->cfg->size)
    {
      return -ENOMSG;
    }

  uint8_t idx = msg.id - nodeid - v->cfg->start;
  SCanIoData *canio = &v->io[idx];

  if (!canio->io)
    {
      return -EINVAL;
    }

#ifdef CONFIG_DAWN_PROTO_CAN_SIMPLE
  if (v->cfg->type == CAN_TYPE_WRITE)
    {
      if (canio->io->isWrite() == false)
        {
          DAWNERR("IO doesn't support write\n");
          return -EINVAL;
        }

      std::memcpy(canio->iodata->getDataPtr(), msg.data, msg.len);

      ret = canio->io->setData(*canio->iodata);
      if (ret != OK)
        {
          DAWNERR("failed to set IO\n");
          return ret;
        }

      return OK;
    }
#endif

#ifdef CONFIG_DAWN_PROTO_CAN_SEG
  if (v->cfg->type == CAN_TYPE_READ_SEG && msg.rtr == 1)
    {
      // Segmented reads do not use RTR.
      return -ENOTSUP;
    }

  if (v->cfg->type == CAN_TYPE_READ_SEG && msg.rtr == 0)
    {
      if (msg.len != 0)
        {
          return OK;
        }

      if (canio->io->isRead() == false)
        {
          DAWNERR("IO doesn't support read\n");
          return -EINVAL;
        }

      return sendIoResponse(canio, 0, true);
    }

  if (v->cfg->type == CAN_TYPE_WRITE_SEG)
    {
      uint64_t now = canTimestampUs();
      uint64_t timeout = CONFIG_DAWN_PROTO_CAN_SEG_TIMEOUT_US;
      size_t payload_len;

      // Allow other handlers (e.g. READ_SEG on same CAN ID) to process
      // non-write traffic.
      if (msg.rtr == 1)
        {
          return -ENOMSG;
        }

      ret = CIsoTp::receive(
        msg, *canio->isotp, canio->iodata->getDataPtr(), canio->data_len, now, timeout);
      if (ret < 0)
        {
          return ret;
        }

      if (ret == 1) // Complete
        {
          size_t old_size;

          payload_len = canio->isotp->total_len;

          if (canio->io->isSeekable())
            {
              // Reuse the staged segmented buffer, but clamp the logical size
              // to the received ISO-TP payload before writing.
              old_size = canio->iodata->N;
              canio->iodata->N = payload_len;
              ret = canio->io->setData(*canio->iodata);
              canio->iodata->N = old_size;
            }
          else
            {
              ret = canio->io->setData(*canio->iodata);
            }

          CIsoTp::resetState(*canio->isotp);
          if (ret != OK)
            {
              DAWNERR("failed to set IO\n");
              return ret;
            }
        }

      return OK;
    }
#endif

#ifdef CONFIG_DAWN_PROTO_CAN_RTR
  if (msg.rtr == 1)
    {
#  ifdef CONFIG_DAWN_PROTO_CAN_SIMPLE
      if (v->cfg->type == CAN_TYPE_READ)
        {
          if (canio->io->isRead() == false)
            {
              DAWNERR("IO doesn't support read\n");
              return -EINVAL;
            }

          return sendIoResponse(canio, 0, false);
        }
#  endif

#  ifdef CONFIG_DAWN_PROTO_CAN_SEG
      if (v->cfg->type == CAN_TYPE_READ_SEG)
        {
          return -ENOTSUP;
        }
#  endif
    }
#endif

  return OK;
}
