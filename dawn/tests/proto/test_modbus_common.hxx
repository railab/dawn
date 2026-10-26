// dawn/tests/proto/test_modbus_common.hxx
//
// SPDX-License-Identifier: Apache-2.0
//

#pragma once

#include <cstddef>
#include <cstdint>

#include "test_common.hxx"

//***************************************************************************
// Description: shared file-record PDU checks for RTU and TCP
//***************************************************************************

static inline void modbus_file_cases(int fd,
                                     int (*exchange)(int, const uint8_t *, size_t, uint8_t *))
{
  const uint8_t write[] = {21, 11, 6, 0, 1, 0, 1, 0, 2, 0x12, 0x34, 0xab, 0xcd};
  const uint8_t multi[] = {20, 14, 6, 0, 1, 0, 0, 0, 1, 6, 0, 1, 0, 3, 0, 1};
  const uint8_t malformed[] = {21,   18, 6, 0, 1, 0, 0, 0, 1,    0xff,
                               0xff, 5,  0, 2, 0, 1, 0, 1, 0xff, 0xff};
  uint8_t response[260];
  uint8_t read[] = {20, 7, 6, 0, 1, 0, 0, 0, 4};
  int i;

  TEST_ASSERT_EQUAL(12, exchange(fd, read, sizeof(read), response));
  TEST_ASSERT_EQUAL(20, response[0]);
  TEST_ASSERT_EQUAL(10, response[1]);
  TEST_ASSERT_EQUAL(9, response[2]);
  TEST_ASSERT_EQUAL(6, response[3]);

  for (i = 0; i < 8; i++)
    {
      TEST_ASSERT_EQUAL(i, response[4 + i]);
    }

  // Repeated reads at the same record return the same data.

  TEST_ASSERT_EQUAL(12, exchange(fd, read, sizeof(read), response));
  TEST_ASSERT_EQUAL(0, response[4]);
  read[6] = 3;
  TEST_ASSERT_EQUAL(12, exchange(fd, read, sizeof(read), response));
  TEST_ASSERT_EQUAL(6, response[4]);

  TEST_ASSERT_EQUAL(sizeof(write), exchange(fd, write, sizeof(write), response));
  TEST_ASSERT_EQUAL_MEMORY(write, response, sizeof(write));
  TEST_ASSERT_EQUAL(sizeof(write), exchange(fd, write, sizeof(write), response));
  read[6] = 1;
  read[8] = 2;
  TEST_ASSERT_EQUAL(8, exchange(fd, read, sizeof(read), response));
  TEST_ASSERT_EQUAL_MEMORY(write + 9, response + 4, 4);

  // Invalid file, zero-filled EOF, record boundary and reference type.

  read[4] = 0;
  TEST_ASSERT_EQUAL(2, exchange(fd, read, sizeof(read), response));
  TEST_ASSERT_EQUAL(0x94, response[0]);
  TEST_ASSERT_EQUAL(2, response[1]);
  read[4] = 1;
  read[6] = 16;
  read[8] = 1;
  TEST_ASSERT_EQUAL(6, exchange(fd, read, sizeof(read), response));
  TEST_ASSERT_EQUAL(0, response[4]);
  TEST_ASSERT_EQUAL(0, response[5]);
  read[5] = 0x27;
  read[6] = 0x0f;
  read[8] = 2;
  TEST_ASSERT_EQUAL(2, exchange(fd, read, sizeof(read), response));
  TEST_ASSERT_EQUAL(2, response[1]);
  read[5] = read[6] = 0;
  read[2] = 5;
  TEST_ASSERT_EQUAL(2, exchange(fd, read, sizeof(read), response));
  TEST_ASSERT_EQUAL(2, response[1]);

  // Two subrequests, with a response that overwrites the request buffer.

  TEST_ASSERT_EQUAL(10, exchange(fd, multi, sizeof(multi), response));
  TEST_ASSERT_EQUAL(0, response[4]);
  TEST_ASSERT_EQUAL(6, response[8]);

  // A malformed later write group must not apply the first group.

  TEST_ASSERT_EQUAL(2, exchange(fd, malformed, sizeof(malformed), response));
  TEST_ASSERT_EQUAL(0x95, response[0]);
  read[2] = 6;
  read[8] = 1;
  TEST_ASSERT_EQUAL(6, exchange(fd, read, sizeof(read), response));
  TEST_ASSERT_EQUAL(0, response[4]);
}
