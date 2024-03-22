// dawn/src/proto/simplebase.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/simplebase.hxx"

#include <cstring>
#include <new>

#include "dawn/io/common.hxx"
#include "dawn/io/sdata.hxx"

using namespace dawn;

static inline uint32_t extractU32LE(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

uint16_t CProtoSimpleBase::calculateCrc(const uint8_t *data, size_t len)
{
  uint16_t crc = 0xFFFF;

  for (size_t i = 0; i < len; i++)
    {
      crc ^= (uint16_t)data[i] << 8;
      for (int j = 0; j < 8; j++)
        {
          if (crc & 0x8000)
            {
              crc = (crc << 1) ^ 0x1021;
            }
          else
            {
              crc <<= 1;
            }
        }
    }

  return crc;
}

void CProtoSimpleBase::sendError(uint8_t error_code, uint8_t context)
{
  uint8_t payload[2] = {error_code, context};
  sendFrame(CMD_ERROR, payload, 2);
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CProtoSimpleBase::notifierCb(void *priv, io_ddata_t *data)
{
  SProtoSimpleNotify *ctx;
  CProtoSimpleBase *proto;
  CIOCommon *io;
  SObjectId::ObjectId objid;
  uint8_t payload[4 + FRAME_MAX_PAYLOAD];
  size_t data_size;

  ctx = (SProtoSimpleNotify *)priv;
  proto = ctx->proto;
  io = ctx->io;
  objid = ctx->objid;

  {
    std::lock_guard<std::mutex> lock(proto->subsMutex);
    if (proto->subscriptions.find(objid) == proto->subscriptions.end())
      {
        return OK;
      }
  }

  data_size = io->getDataSize();

  if (data_size > FRAME_MAX_PAYLOAD - 4)
    {
      DAWNERR("Data too large for notification\n");
      return -EINVAL;
    }

  payload[0] = (objid >> 0) & 0xFF;
  payload[1] = (objid >> 8) & 0xFF;
  payload[2] = (objid >> 16) & 0xFF;
  payload[3] = (objid >> 24) & 0xFF;

  std::memcpy(&payload[4], data->getDataPtr(), data_size);

  return proto->sendFrame(CMD_NOTIFY, payload, 4 + data_size);
}
#endif

void CProtoSimpleBase::cmdPing()
{
  sendFrame(CMD_PONG, nullptr, 0);
}

io_ddata_t *CProtoSimpleBase::findIOBuffer(SObjectId::ObjectId objid)
{
  for (auto *v : iobuffers)
    {
      if (objid == v->cfg->objid)
        {
          return v->iodata;
        }
    }

  return nullptr;
}

void CProtoSimpleBase::cmdGetIO(const uint8_t *payload, size_t len)
{
  const uint8_t *response;
  SObjectId::ObjectId objid;
  CIOCommon *io = NULL;
  io_ddata_t *iodata = NULL;
  size_t data_size;
  int ret;

  if (len < 4)
    {
      sendError(STATUS_INVALID_FORMAT, CMD_GET_IO);
      return;
    }

  objid = extractU32LE(payload);

  io = getIO(objid);
  if (!io)
    {
      sendError(STATUS_INVALID_OBJ, CMD_GET_IO);
      return;
    }

  if (!io->isRead())
    {
      sendError(STATUS_WRITE_ONLY, CMD_GET_IO);
      return;
    }

  iodata = findIOBuffer(objid);
  if (!iodata)
    {
      DAWNERR("Not found IO buffer\n");
      sendError(STATUS_ERROR, CMD_GET_IO);
      return;
    }

  ret = io->getData(*iodata, 1);
  if (ret < 0)
    {
      DAWNERR("get data failed\n");
      sendError(STATUS_ERROR, CMD_GET_IO);
      return;
    }

  data_size = io->getDataSize();
  if (data_size > FRAME_MAX_PAYLOAD)
    {
      sendError(STATUS_ERROR, CMD_GET_IO);
      return;
    }

  response = static_cast<const uint8_t *>(iodata->getDataPtr());
  sendFrame(CMD_GET_IO, response, data_size);
}

void CProtoSimpleBase::cmdSetIO(const uint8_t *payload, size_t len)
{
  SObjectId::ObjectId objid;
  CIOCommon *io = NULL;
  io_ddata_t *iodata = NULL;
  uint8_t status;
  size_t size;
  int ret;

  if (len < 5)
    {
      sendError(STATUS_INVALID_FORMAT, CMD_SET_IO);
      return;
    }

  objid = extractU32LE(payload);

  io = getIO(objid);
  if (!io)
    {
      sendError(STATUS_INVALID_OBJ, CMD_SET_IO);
      return;
    }

  if (!io->isWrite())
    {
      sendError(STATUS_READ_ONLY, CMD_SET_IO);
      return;
    }

  iodata = findIOBuffer(objid);
  if (!iodata)
    {
      DAWNERR("Not found IO buffer\n");
      sendError(STATUS_ERROR, CMD_SET_IO);
      return;
    }

  if (io->isSeekable())
    {
      size_t old_size;

      if (len <= 4)
        {
          sendError(STATUS_INVALID_FORMAT, CMD_SET_IO);
          return;
        }

      size = len - 4;
      if (size > iodata->getDataSize())
        {
          sendError(STATUS_ERROR, CMD_SET_IO);
          return;
        }

      // Reuse the preallocated seek buffer, but expose only the received
      // payload length to the seekable IO write handler.
      old_size = iodata->N;
      iodata->N = size;
      std::memcpy(iodata->getDataPtr(), &payload[4], size);
      ret = io->setData(*iodata);
      iodata->N = old_size;
    }
  else
    {
      size = io->getDataSize();
      if (len != 4 + size)
        {
          sendError(STATUS_ERROR, CMD_SET_IO);
          return;
        }

      std::memcpy(iodata->getDataPtr(), &payload[4], size);
      ret = io->setData(*iodata);
    }

  status = (ret == 0) ? STATUS_OK : STATUS_ERROR;
  sendFrame(CMD_SET_IO, &status, 1);
}

void CProtoSimpleBase::cmdGetIOSeek(const uint8_t *payload, size_t len)
{
  uint8_t response[FRAME_MAX_PAYLOAD];
  SObjectId::ObjectId objid;
  CIOCommon *io = NULL;
  io_ddata_t *iodata = NULL;
  size_t total;
  size_t offset;
  size_t avail;
  size_t chunk;
  int ret;

  if (len < 8)
    {
      sendError(STATUS_INVALID_FORMAT, CMD_GET_IO_SEEK);
      return;
    }

  objid = extractU32LE(payload);
  offset = extractU32LE(&payload[4]);

  io = getIO(objid);
  if (!io)
    {
      sendError(STATUS_INVALID_OBJ, CMD_GET_IO_SEEK);
      return;
    }

  if (!io->isRead())
    {
      sendError(STATUS_WRITE_ONLY, CMD_GET_IO_SEEK);
      return;
    }

  if (!io->isSeekable())
    {
      sendError(STATUS_ERROR, CMD_GET_IO_SEEK);
      return;
    }

  total = io->getDataSize();
  if (offset >= total)
    {
      sendError(STATUS_INVALID_FORMAT, CMD_GET_IO_SEEK);
      return;
    }

  iodata = findIOBuffer(objid);
  if (!iodata)
    {
      DAWNERR("Not found IO buffer\n");
      sendError(STATUS_ERROR, CMD_GET_IO_SEEK);
      return;
    }

  ret = io->getData(*iodata, 1, offset);
  if (ret < 0)
    {
      DAWNERR("get data seek failed %d\n", ret);
      sendError(STATUS_ERROR, CMD_GET_IO_SEEK);
      return;
    }

  avail = total - offset;
  chunk = iodata->getDataSize() < avail ? iodata->getDataSize() : avail;

  response[0] = (uint8_t)(objid & 0xFF);
  response[1] = (uint8_t)((objid >> 8) & 0xFF);
  response[2] = (uint8_t)((objid >> 16) & 0xFF);
  response[3] = (uint8_t)((objid >> 24) & 0xFF);

  response[4] = (uint8_t)(total & 0xFF);
  response[5] = (uint8_t)((total >> 8) & 0xFF);
  response[6] = (uint8_t)((total >> 16) & 0xFF);
  response[7] = (uint8_t)((total >> 24) & 0xFF);

  std::memcpy(&response[SEEK_HDR_SIZE], iodata->getDataPtr(), chunk);

  sendFrame(CMD_GET_IO_SEEK, response, SEEK_HDR_SIZE + chunk);
}

void CProtoSimpleBase::cmdSetIOSeek(const uint8_t *payload, size_t len)
{
  SObjectId::ObjectId objid;
  CIOCommon *io = NULL;
  io_ddata_t *iodata = NULL;
  uint8_t status;
  size_t offset;
  size_t size;
  size_t old_size;
  int ret;

  if (len < 9)
    {
      sendError(STATUS_INVALID_FORMAT, CMD_SET_IO_SEEK);
      return;
    }

  objid = extractU32LE(payload);
  offset = extractU32LE(&payload[4]);
  size = len - 8;

  io = getIO(objid);
  if (!io)
    {
      sendError(STATUS_INVALID_OBJ, CMD_SET_IO_SEEK);
      return;
    }

  if (!io->isWrite())
    {
      sendError(STATUS_READ_ONLY, CMD_SET_IO_SEEK);
      return;
    }

  if (!io->isSeekable())
    {
      sendError(STATUS_ERROR, CMD_SET_IO_SEEK);
      return;
    }

  iodata = findIOBuffer(objid);
  if (!iodata)
    {
      DAWNERR("Not found IO buffer\n");
      sendError(STATUS_ERROR, CMD_SET_IO_SEEK);
      return;
    }

  if (size > iodata->getDataSize())
    {
      sendError(STATUS_ERROR, CMD_SET_IO_SEEK);
      return;
    }

  old_size = iodata->N;
  iodata->N = size;
  std::memcpy(iodata->getDataPtr(), &payload[8], size);
  ret = io->setData(*iodata, offset);
  iodata->N = old_size;

  status = (ret == 0) ? STATUS_OK : STATUS_ERROR;
  sendFrame(CMD_SET_IO_SEEK, &status, 1);
}

void CProtoSimpleBase::cmdGetCfg(const uint8_t *payload, size_t len)
{
  SObjectId::ObjectId objid;
  uint8_t cfgid;
  CIOCommon *io;

  if (len < 5)
    {
      sendError(STATUS_INVALID_CFG, CMD_GET_CFG);
      return;
    }

  objid = extractU32LE(payload);

  cfgid = payload[4];
  UNUSED(cfgid);

  io = getIO(objid);
  if (!io)
    {
      sendError(STATUS_INVALID_OBJ, CMD_GET_CFG);
      return;
    }

  sendError(STATUS_INVALID_CFG, CMD_GET_CFG);
}

void CProtoSimpleBase::cmdSetCfg(const uint8_t *payload, size_t len)
{
  SObjectId::ObjectId objid;
  uint8_t cfgid;
  CIOCommon *io;

  if (len < 5)
    {
      sendError(STATUS_INVALID_CFG, CMD_SET_CFG);
      return;
    }

  objid = extractU32LE(payload);

  cfgid = payload[4];
  UNUSED(cfgid);

  io = getIO(objid);
  if (!io)
    {
      sendError(STATUS_INVALID_OBJ, CMD_SET_CFG);
      return;
    }

  sendError(STATUS_INVALID_CFG, CMD_SET_CFG);
}

void CProtoSimpleBase::cmdGetInfo(const uint8_t *payload, size_t len)
{
  SObjectId::ObjectId objid;
  uint8_t response[3];
  CIOCommon *io;

  if (len < 4)
    {
      sendError(STATUS_INVALID_FORMAT, CMD_GET_INFO);
      return;
    }

  objid = extractU32LE(payload);

  io = getIO(objid);
  if (!io)
    {
      sendError(STATUS_INVALID_OBJ, CMD_GET_INFO);
      return;
    }

  if (io->isRead() && io->isWrite())
    {
      response[0] = IO_TYPE_READ_WRITE;
    }
  else if (io->isRead())
    {
      response[0] = IO_TYPE_READ_ONLY;
    }
  else if (io->isWrite())
    {
      response[0] = IO_TYPE_WRITE_ONLY;
    }
  else
    {
      sendError(STATUS_INVALID_OBJ, CMD_GET_INFO);
      return;
    }

  response[1] = (uint8_t)io->getDataDim();
  response[2] = (uint8_t)io->getDtype();

  sendFrame(CMD_GET_INFO, response, 3);
}

void CProtoSimpleBase::cmdListIOs()
{
  uint8_t response[2 + FRAME_MAX_PAYLOAD - 2];
  size_t resp_len = 0;
  uint16_t count = (uint16_t)getIOMap().size();

  response[resp_len++] = (uint8_t)(count & 0xFF);
  response[resp_len++] = (uint8_t)((count >> 8) & 0xFF);

  for (const auto &entry : getIOMap())
    {
      SObjectId::ObjectId objid;

      if (resp_len + 4 > FRAME_MAX_PAYLOAD)
        {
          sendError(STATUS_ERROR, CMD_LIST_IOS);
          return;
        }

      objid = entry.first;

      response[resp_len++] = (uint8_t)(objid & 0xFF);
      response[resp_len++] = (uint8_t)((objid >> 8) & 0xFF);
      response[resp_len++] = (uint8_t)((objid >> 16) & 0xFF);
      response[resp_len++] = (uint8_t)((objid >> 24) & 0xFF);
    }

  sendFrame(CMD_LIST_IOS, response, resp_len);
}

#ifdef CONFIG_DAWN_IO_NOTIFY
void CProtoSimpleBase::cmdSubscribe(const uint8_t *payload, size_t len)
{
  SObjectId::ObjectId objid;
  CIOCommon *io;
  uint8_t status;

  if (len < 4)
    {
      sendError(STATUS_INVALID_FORMAT, CMD_SUBSCRIBE);
      return;
    }

  objid = extractU32LE(payload);

  io = getIO(objid);
  if (!io)
    {
      sendError(STATUS_INVALID_OBJ, CMD_SUBSCRIBE);
      return;
    }

  if (!io->isNotify())
    {
      sendError(STATUS_ERROR, CMD_SUBSCRIBE);
      return;
    }

  {
    std::lock_guard<std::mutex> lock(subsMutex);
    subscriptions.insert(objid);
  }

  status = STATUS_OK;
  sendFrame(CMD_SUBSCRIBE, &status, 1);
}

void CProtoSimpleBase::cmdUnsubscribe(const uint8_t *payload, size_t len)
{
  SObjectId::ObjectId objid;
  uint8_t status;

  if (len < 4)
    {
      sendError(STATUS_INVALID_FORMAT, CMD_UNSUBSCRIBE);
      return;
    }

  objid = extractU32LE(payload);

  {
    std::lock_guard<std::mutex> lock(subsMutex);
    subscriptions.erase(objid);
  }

  status = STATUS_OK;
  sendFrame(CMD_UNSUBSCRIBE, &status, 1);
}
#endif

int CProtoSimpleBase::handleFrame(const uint8_t *frame, size_t len)
{
  const uint8_t *payload;
  uint16_t payload_len;
  uint8_t cmd;
  uint16_t crc_calc;
  uint16_t crc_recv;

  if (len < FRAME_MIN_LEN)
    {
      return -1;
    }

  if (frame[0] != FRAME_SYNC)
    {
      return -1;
    }

  payload_len = (uint16_t)(frame[1] | (frame[2] << 8));
  cmd = frame[3];

  if (len != (size_t)(FRAME_MIN_LEN + payload_len))
    {
      return -1;
    }

  crc_calc = calculateCrc(&frame[3], 1 + payload_len);
  crc_recv = (uint16_t)(frame[4 + payload_len] | (frame[5 + payload_len] << 8));

  if (crc_calc != crc_recv)
    {
      return -1;
    }

  payload = &frame[4];

  switch (cmd)
    {
      case CMD_PING:
        {
          cmdPing();
          break;
        }

      case CMD_GET_IO:
        {
          cmdGetIO(payload, payload_len);
          break;
        }

      case CMD_SET_IO:
        {
          cmdSetIO(payload, payload_len);
          break;
        }

      case CMD_GET_IO_SEEK:
        {
          cmdGetIOSeek(payload, payload_len);
          break;
        }

      case CMD_SET_IO_SEEK:
        {
          cmdSetIOSeek(payload, payload_len);
          break;
        }

      case CMD_GET_CFG:
        {
          cmdGetCfg(payload, payload_len);
          break;
        }

      case CMD_SET_CFG:
        {
          cmdSetCfg(payload, payload_len);
          break;
        }

      case CMD_GET_INFO:
        {
          cmdGetInfo(payload, payload_len);
          break;
        }

      case CMD_LIST_IOS:
        {
          cmdListIOs();
          break;
        }

#ifdef CONFIG_DAWN_IO_NOTIFY
      case CMD_SUBSCRIBE:
        {
          cmdSubscribe(payload, payload_len);
          break;
        }

      case CMD_UNSUBSCRIBE:
        {
          cmdUnsubscribe(payload, payload_len);
          break;
        }
#endif

      default:
        {
          sendError(STATUS_INVALID_CMD, cmd);
          break;
        }
    }

  return 0;
}

void CProtoSimpleBase::allocObject(SProtoSimpleIOBind *cfg)
{
  DAWNINFO("allocate object 0x%" PRIx32 "\n", cfg->objid);

  setObjectMapItem(cfg->objid, nullptr);
  iobinds.push_back(cfg);
}

int CProtoSimpleBase::createBuffers()
{
  if (initialized)
    {
      return OK;
    }

  for (auto const *v : iobinds)
    {
      CIOCommon *io;
      SProtoSimpleData *tmp;
      io_ddata_t *iobuffer;

      io = getIO(v->objid);
      if (io == nullptr)
        {
          DAWNERR("Failed to get IO 0x%08" PRIx32 "\n", v->objid);
          return -EIO;
        }

      tmp = new (std::nothrow) SProtoSimpleData[1]();
      if (!tmp)
        {
          DAWNERR("failed to allocate data\n");
          return -ENOMEM;
        }

      iobuffers.push_back(tmp);
      tmp->cfg = v;

      iobuffer = io->ddata_alloc(1, SEEK_CHUNK_CAP);
      if (!iobuffer)
        {
          DAWNERR("failed to allocate register buffer\n");
          return -ENOMEM;
        }

      tmp->iodata = iobuffer;
    }

  initialized = true;

  return OK;
}

int CProtoSimpleBase::destroyBuffers()
{
  if (!initialized)
    {
      return OK;
    }

  for (auto *v : iobuffers)
    {
      free(v->iodata);
      delete[] v;
    }

  iobinds.clear();
  iobuffers.clear();
  initialized = false;

  return OK;
}

#ifdef CONFIG_DAWN_IO_NOTIFY
int CProtoSimpleBase::setupNotifications()
{
  int ret;

  for (auto *v : iobuffers)
    {
      CIOCommon *io;
      SProtoSimpleNotify *ctx;

      io = getIO(v->cfg->objid);
      if (!io)
        {
          continue;
        }

      if (io->isNotify())
        {
          ctx = new (std::nothrow) SProtoSimpleNotify[1]();
          if (!ctx)
            {
              DAWNERR("Failed to allocate notification context\n");
              return -ENOMEM;
            }

          ctx->objid = v->cfg->objid;
          ctx->io = io;
          ctx->proto = this;

          notifyContexts.push_back(ctx);

          ret = io->setNotifier(notifierCb, 0, ctx);
          if (ret < 0)
            {
              DAWNERR("setNotifier failed for objid=0x%" PRIx32 "\n", ctx->objid);
              delete[] ctx;
              notifyContexts.pop_back();
            }
        }
    }

  return OK;
}

void CProtoSimpleBase::cleanupNotifications()
{
  {
    std::lock_guard<std::mutex> lock(subsMutex);
    subscriptions.clear();
  }
}

void CProtoSimpleBase::destroyNotifications()
{
  cleanupNotifications();

  for (auto *ctx : notifyContexts)
    {
      delete[] ctx;
    }

  notifyContexts.clear();
}
#endif
