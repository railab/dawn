// dawn/tests/proto/test_simple_proto_common.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <cstdint>

#include <cstring>

#define TEST_SIMPLE_PROTO_FRAME_MIN_LEN     6
#define TEST_SIMPLE_PROTO_FRAME_MAX_PAYLOAD 1024

static inline uint16_t test_simple_proto_calculate_crc(uint8_t cmd,
                                                       const uint8_t *payload,
                                                       size_t len)
{
  uint16_t crc;
  uint8_t data;

  crc = 0xFFFF;

  data = cmd;
  crc ^= (uint16_t)data << 8;
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

  for (size_t i = 0; i < len; i++)
    {
      data = payload[i];
      crc ^= (uint16_t)data << 8;
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

static inline size_t test_simple_proto_build_frame(uint8_t cmd,
                                                   const uint8_t *payload,
                                                   size_t len,
                                                   uint8_t *frame)
{
  size_t pos;
  uint16_t crc;

  pos = 0;

  frame[pos++] = 0xAA;
  frame[pos++] = (uint8_t)(len & 0xFF);
  frame[pos++] = (uint8_t)((len >> 8) & 0xFF);
  frame[pos++] = cmd;

  if (len > 0)
    {
      std::memcpy(&frame[pos], payload, len);
      pos += len;
    }

  crc = test_simple_proto_calculate_crc(cmd, payload, len);

  frame[pos++] = (uint8_t)(crc & 0xFF);
  frame[pos++] = (uint8_t)((crc >> 8) & 0xFF);

  return pos;
}

static inline int test_simple_proto_parse_frame(const uint8_t *frame,
                                                size_t frame_len,
                                                uint8_t *cmd,
                                                uint8_t *payload,
                                                size_t *len)
{
  uint16_t payload_len;
  uint16_t crc;
  uint16_t calc_crc;

  if (frame_len < TEST_SIMPLE_PROTO_FRAME_MIN_LEN)
    {
      return -1;
    }

  if (frame[0] != 0xAA)
    {
      return -1;
    }

  payload_len = (uint16_t)(frame[1] | (frame[2] << 8));
  if (frame_len != (size_t)(TEST_SIMPLE_PROTO_FRAME_MIN_LEN + payload_len))
    {
      return -1;
    }

  *cmd = frame[3];

  if (payload_len > 0)
    {
      std::memcpy(payload, &frame[4], payload_len);
    }

  calc_crc = test_simple_proto_calculate_crc(*cmd, payload, payload_len);
  crc = (uint16_t)(frame[4 + payload_len] | (frame[5 + payload_len] << 8));
  if (calc_crc != crc)
    {
      return -1;
    }

  *len = payload_len;

  return 0;
}
