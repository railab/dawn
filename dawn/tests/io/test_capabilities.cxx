// dawn/tests/io/test_capabilities.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>

#include <cstdint>
#include <cstring>

#include "dawn/io/capabilities.hxx"
#include "dawn/io/ddata.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint16_t readle16(const uint8_t *p)
{
  return static_cast<uint16_t>(p[0]) | (static_cast<uint16_t>(p[1]) << 8);
}

static uint32_t readle32(const uint8_t *p)
{
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8) |
         (static_cast<uint32_t>(p[2]) << 16) | (static_cast<uint32_t>(p[3]) << 24);
}

//***************************************************************************
// Description: capabilities IO exposes the full read-only capability blob.
//***************************************************************************

static void test_io_capabilities_blob()
{
  uint32_t cfg[] = {CIOCapabilities::objectId(0), 0};
  CDescObject desc(cfg);
  CIOCapabilities io(desc);
  io_ddata_t *data;
  uint8_t *ptr;
  uint16_t payloadLen;
  uint32_t ioW1;
  uint32_t ioW2;
  uint32_t descSlots;
  uint32_t slotSize;
  int ret;

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  ret = io.init();
  TEST_ASSERT_EQUAL(OK, ret);

  TEST_ASSERT_EQUAL(true, io.isSeekable());
  TEST_ASSERT_EQUAL(true, io.isRead());
  TEST_ASSERT_EQUAL(false, io.isWrite());
  TEST_ASSERT_EQUAL(512u, io.getDataSize());

  data = io.ddata_alloc(1, io.getDataSize());
  TEST_ASSERT(data != nullptr);

  ret = io.getData(*data, 1, 0);
  TEST_ASSERT_EQUAL(OK, ret);

  ptr = static_cast<uint8_t *>(data->getDataPtr());
  TEST_ASSERT_EQUAL(2, ptr[0]); // version
  TEST_ASSERT_EQUAL(0, ptr[1]); // reserved/layout
  payloadLen = readle16(&ptr[2]);
  TEST_ASSERT_EQUAL(504, payloadLen);
  TEST_ASSERT_EQUAL(0u, readle32(&ptr[4]));

  // IO section: verify capabilities class bit (53 -> word 1, bit 21).
  ioW1 = readle32(&ptr[8 + 4]);
  TEST_ASSERT((ioW1 & (1u << 21)) != 0u);

  // check next word
  ioW2 = readle32(&ptr[8 + 8]);
  TEST_ASSERT((ioW2 & 1u) != 0u);

  // Metadata section starts after 3x64-byte sections.
  descSlots = readle32(&ptr[8 + 192 + 24]);
  slotSize = readle32(&ptr[8 + 192 + 28]);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_DESC_SLOTS, descSlots);
  TEST_ASSERT_EQUAL(CONFIG_DAWN_DESC_SLOT_SIZE, slotSize);

  ret = io.setData(*data, 0);
  TEST_ASSERT_EQUAL(-ENOTSUP, ret);

  free(data);
}

//***************************************************************************
// Description: capability blob reads can be split into seekable chunks.
//***************************************************************************

static void test_io_capabilities_seekable_chunks()
{
  uint32_t cfg[] = {CIOCapabilities::objectId(0), 0};
  CDescObject desc(cfg);
  CIOCapabilities io(desc);
  io_ddata_t *full;
  io_ddata_t *chunk;
  uint8_t first8[8];
  int ret;

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  ret = io.init();
  TEST_ASSERT_EQUAL(OK, ret);

  full = io.ddata_alloc(1, io.getDataSize());
  TEST_ASSERT(full != nullptr);
  ret = io.getData(*full, 1, 0);
  TEST_ASSERT_EQUAL(OK, ret);

  chunk = io.ddata_alloc(1, 4);
  TEST_ASSERT(chunk != nullptr);

  ret = io.getData(*chunk, 1, 0);
  TEST_ASSERT_EQUAL(OK, ret);
  std::memcpy(&first8[0], chunk->getDataPtr(), 4);

  ret = io.getData(*chunk, 1, 4);
  TEST_ASSERT_EQUAL(OK, ret);
  std::memcpy(&first8[4], chunk->getDataPtr(), 4);

  TEST_ASSERT_EQUAL_UINT8_ARRAY(static_cast<uint8_t *>(full->getDataPtr()), first8, sizeof(first8));

  free(chunk);
  free(full);
}

//***************************************************************************
// Description: capability reads reject offsets at the end of the blob.
//***************************************************************************

static void test_io_capabilities_out_of_range_offset()
{
  uint32_t cfg[] = {CIOCapabilities::objectId(0), 0};
  CDescObject desc(cfg);
  CIOCapabilities io(desc);
  io_ddata_t *data;
  int ret;

  ret = io.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  ret = io.init();
  TEST_ASSERT_EQUAL(OK, ret);

  data = io.ddata_alloc(1, 4);
  TEST_ASSERT(data != nullptr);

  ret = io.getData(*data, 1, io.getDataSize());
  TEST_ASSERT_EQUAL(-EINVAL, ret);

  free(data);
}

extern "C"
{
  int test_io_capabilities()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_capabilities_blob);
    DAWN_RUN_TEST(test_io_capabilities_seekable_chunks);
    DAWN_RUN_TEST(test_io_capabilities_out_of_range_offset);

    return UNITY_END();
  }
}
