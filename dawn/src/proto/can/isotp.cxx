// dawn/src/proto/can/isotp.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/proto/can/isotp.hxx"

#include <cstring>
#include <errno.h>

#include "dawn/porting/can.hxx"

using namespace dawn;

//***************************************************************************
// Public Methods
//***************************************************************************

void CIsoTp::initState(State &state)
{
  state.offset = 0;
  state.seq_next = 0;
  state.total_len = 0;
  state.active = false;
  state.timestamp = 0;
}

void CIsoTp::resetState(State &state)
{
  state.offset = 0;
  state.seq_next = 0;
  state.total_len = 0;
  state.active = false;
}

uint8_t CIsoTp::getFrameType(uint8_t pci)
{
  return pci & FRAME_TYPE_MASK;
}

uint8_t CIsoTp::getSequence(uint8_t pci)
{
  return pci & SEQ_MASK;
}

uint8_t CIsoTp::nextSequence(uint8_t seq)
{
  return (seq + 1) & SEQ_MASK;
}

int CIsoTp::handleFirstFrame(const porting::canmsg_s &msg,
                             State &state,
                             void *data_buf,
                             size_t buf_size)
{
  size_t total_len;
  size_t payload_len;
  const uint8_t *payload;

  if (msg.len < 2)
    {
      return -EINVAL;
    }

  // Extract total length from FF: [0x10][LL]

  total_len = msg.data[1];
  if (total_len == 0 || total_len > buf_size)
    {
      return -EINVAL;
    }

  // Payload starts at byte 2

  payload = msg.data + 2;
  payload_len = msg.len - 2;

  if (payload_len > total_len)
    {
      payload_len = total_len;
    }

  // Initialize state

  state.total_len = total_len;
  state.offset = 0;
  state.seq_next = 0;
  state.active = true;

  // Copy first chunk

  std::memcpy(data_buf, payload, payload_len);
  state.offset = payload_len;

  return static_cast<int>(payload_len);
}

int CIsoTp::handleConsecutiveFrame(const porting::canmsg_s &msg,
                                   State &state,
                                   void *data_buf,
                                   size_t buf_size)
{
  uint8_t seq;
  size_t payload_len;
  const uint8_t *payload;
  size_t remaining;

  if (!state.active)
    {
      return -EINVAL;
    }

  if (msg.len < 1)
    {
      return -EINVAL;
    }

  // Check sequence number

  seq = getSequence(msg.data[0]);
  if (seq != state.seq_next)
    {
      resetState(state);
      return -EINVAL;
    }

  // Payload starts at byte 1

  payload = msg.data + 1;
  payload_len = msg.len - 1;

  // Check buffer overflow

  remaining = state.total_len - state.offset;
  if (payload_len > remaining)
    {
      payload_len = remaining;
    }

  if (state.offset + payload_len > buf_size)
    {
      resetState(state);
      return -EINVAL;
    }

  // Copy data

  std::memcpy(static_cast<uint8_t *>(data_buf) + state.offset, payload, payload_len);

  state.offset += payload_len;
  state.seq_next = nextSequence(state.seq_next);

  // Check if complete

  if (state.offset >= state.total_len)
    {
      return 1; // Transfer complete
    }

  return static_cast<int>(payload_len);
}

bool CIsoTp::isComplete(const State &state)
{
  return state.active && (state.offset >= state.total_len);
}

bool CIsoTp::isTimeout(const State &state, uint64_t now, uint64_t timeout)
{
  if (!state.active || timeout == 0)
    {
      return false;
    }

  return (now - state.timestamp) > timeout;
}

int CIsoTp::receive(const porting::canmsg_s &msg,
                    State &state,
                    void *data_buf,
                    size_t buf_size,
                    uint64_t now,
                    uint64_t timeout)
{
  uint8_t frame_type;
  int ret;

  if (msg.len < 1)
    {
      return -ENOMSG;
    }

  frame_type = getFrameType(msg.data[0]);

  if (frame_type != FRAME_FF && frame_type != FRAME_CF)
    {
      return -ENOMSG;
    }

  // Check timeout

  if (isTimeout(state, now, timeout))
    {
      resetState(state);
      return -ETIMEDOUT;
    }

  // Handle First Frame

  if (frame_type == FRAME_FF)
    {
      ret = handleFirstFrame(msg, state, data_buf, buf_size);
      if (ret < 0)
        {
          return ret;
        }

      state.timestamp = now;
      return 0; // In progress
    }

  // Handle Consecutive Frame

  ret = handleConsecutiveFrame(msg, state, data_buf, buf_size);
  if (ret < 0)
    {
      return ret;
    }

  state.timestamp = now;

  // Check if complete

  if (ret == 1)
    {
      return 1; // Complete
    }

  return 0;     // In progress
}

int CIsoTp::receiveCustom(const porting::canmsg_s &msg,
                          State &state,
                          void *data_buf,
                          size_t buf_size,
                          uint8_t seg_no,
                          bool seg_last,
                          uint64_t now,
                          uint64_t timeout)
{
  size_t payload_len = msg.len - 2;

  if (!state.active)
    {
      if (seg_no != 0)
        {
          return -EINVAL;
        }

      state.active = true;
      state.seq_next = 0;
      state.offset = 0;
      state.timestamp = now;
    }
  else if (timeout > 0 && (now - state.timestamp) > timeout)
    {
      state.active = false;
      if (seg_no != 0)
        {
          return -ETIMEDOUT;
        }

      state.active = true;
      state.seq_next = 0;
      state.offset = 0;
      state.timestamp = now;
    }

  if (seg_no != state.seq_next)
    {
      state.active = false;
      return -EINVAL;
    }

  if (state.offset + payload_len > buf_size)
    {
      state.active = false;
      return -EINVAL;
    }

  std::memcpy(static_cast<uint8_t *>(data_buf) + state.offset, msg.data + 2, payload_len);

  state.offset += payload_len;
  state.timestamp = now;

  if (seg_last)
    {
      if (state.offset != buf_size)
        {
          state.active = false;
          return -EINVAL;
        }

      resetState(state);
      return 1; // Complete
    }

  state.seq_next += 1;
  return 0; // In progress
}
