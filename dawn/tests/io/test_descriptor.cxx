// dawn/tests/io/test_descriptor.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <errno.h>

#include "dawn/dev/descriptor.hxx"
#include "dawn/io/ddata.hxx"
#include "dawn/io/descriptor.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: test descriptors for DescriptorIO
//***************************************************************************

static uint32_t g_cfg_descriptor1[] = {
  CIODescriptor::objectId(0),
  0,
};

#if CONFIG_DAWN_DESC_SLOTS > 1
static uint32_t g_cfg_descriptor2[] = {
  CIODescriptor::objectId(1),
  0,
};
#endif

static uint32_t g_dawn_desc1[] = {
  1,
  2,
  3,
  4,
  5,
};

#if CONFIG_DAWN_DESC_SLOTS > 1
static uint32_t g_dawn_desc2[] = {
  0xde,
  0xad,
  0xbe,
  0xef,
};
#endif

// Register g_dawn_desc1 in slot 0 and configure+init the supplied IO.

static void descriptor_setup_slot0(CDevDescriptor &dev, CIODescriptor &io)
{
  CDevDescriptor::SDescriptorReg reg;

  reg.ptr = (void *)g_dawn_desc1;
  reg.len = sizeof(g_dawn_desc1);
  TEST_ASSERT_EQUAL(OK, dev.regDescriptor(0, reg));

  io.setDevDescriptor(&dev);
  TEST_ASSERT_EQUAL(OK, io.configure());
  TEST_ASSERT_EQUAL(OK, io.init());
}

//***************************************************************************
// Description: a full-descriptor read returns the registered payload.
//***************************************************************************

static void test_io_descriptor_read_full_slot0()
{
  CDevDescriptor dev;
  CDescObject desc0(g_cfg_descriptor1);
  CIODescriptor io0(desc0);
  io_ddata_t *data;
  uint32_t *ptr;
  size_t desc_size;

  descriptor_setup_slot0(dev, io0);

  desc_size = io0.getDataSize();
  data = io0.ddata_alloc(1, desc_size);
  TEST_ASSERT(data != nullptr);

  TEST_ASSERT_EQUAL(OK, io0.getData(*data, 1));
  ptr = static_cast<uint32_t *>(data->getDataPtr());
  TEST_ASSERT_EQUAL(1, ptr[0]);
  TEST_ASSERT_EQUAL(2, ptr[1]);
  TEST_ASSERT_EQUAL(3, ptr[2]);
  TEST_ASSERT_EQUAL(4, ptr[3]);
  TEST_ASSERT_EQUAL(5, ptr[4]);

  free(data);
}

//***************************************************************************
// Description: configure() is idempotent (calling it twice still succeeds).
//***************************************************************************

static void test_io_descriptor_configure_idempotent()
{
  CDevDescriptor dev;
  CDevDescriptor::SDescriptorReg reg;
  CDescObject desc0(g_cfg_descriptor1);
  CIODescriptor io0(desc0);

  reg.ptr = (void *)g_dawn_desc1;
  reg.len = sizeof(g_dawn_desc1);
  TEST_ASSERT_EQUAL(OK, dev.regDescriptor(0, reg));

  io0.setDevDescriptor(&dev);
  TEST_ASSERT_EQUAL(OK, io0.configure());
  TEST_ASSERT_EQUAL(OK, io0.configure());
  TEST_ASSERT_EQUAL(OK, io0.init());
}

//***************************************************************************
// Description: a seekable IO returns nullptr from ddata_alloc(dim) without
// a chunk size.
//***************************************************************************

static void test_io_descriptor_alloc_no_chunk_returns_null()
{
  CDevDescriptor dev;
  CDescObject desc0(g_cfg_descriptor1);
  CIODescriptor io0(desc0);

  descriptor_setup_slot0(dev, io0);

  TEST_ASSERT_EQUAL(nullptr, io0.ddata_alloc(1));
}

#if CONFIG_DAWN_DESC_SLOTS > 1
//***************************************************************************
// Description: a slot-1 IO before regDescriptor returns nullptr from
// ddata_alloc.
//***************************************************************************

static void test_io_descriptor_unregistered_alloc_null()
{
  CDescObject desc1(g_cfg_descriptor2);
  CIODescriptor io1(desc1);

  TEST_ASSERT_EQUAL(nullptr, io1.ddata_alloc(1));
  TEST_ASSERT_EQUAL(nullptr, io1.ddata_alloc(1));
}

//***************************************************************************
// Description: a full-descriptor read on slot 1 returns the registered
// payload.
//***************************************************************************

static void test_io_descriptor_read_full_slot1()
{
  CDevDescriptor dev;
  CDevDescriptor::SDescriptorReg reg;
  CDescObject desc1(g_cfg_descriptor2);
  CIODescriptor io1(desc1);
  io_ddata_t *data;
  uint32_t *ptr;
  size_t desc_size;

  reg.ptr = (void *)g_dawn_desc2;
  reg.len = sizeof(g_dawn_desc2);
  TEST_ASSERT_EQUAL(OK, dev.regDescriptor(1, reg));

  io1.setDevDescriptor(&dev);
  TEST_ASSERT_EQUAL(OK, io1.configure());
  TEST_ASSERT_EQUAL(OK, io1.init());

  desc_size = io1.getDataSize();
  data = io1.ddata_alloc(1, desc_size);
  TEST_ASSERT(data != nullptr);

  TEST_ASSERT_EQUAL(OK, io1.getData(*data, 1));
  ptr = static_cast<uint32_t *>(data->getDataPtr());
  TEST_ASSERT_EQUAL(0xde, ptr[0]);
  TEST_ASSERT_EQUAL(0xad, ptr[1]);
  TEST_ASSERT_EQUAL(0xbe, ptr[2]);
  TEST_ASSERT_EQUAL(0xef, ptr[3]);

  free(data);
}
#endif

//***************************************************************************
// Description: a slot-0 descriptor IO reports isSeekable() == true.
//***************************************************************************

static void test_io_descriptor_is_seekable()
{
  CDevDescriptor dev;
  CDescObject desc0(g_cfg_descriptor1);
  CIODescriptor io0(desc0);

  descriptor_setup_slot0(dev, io0);
  TEST_ASSERT_EQUAL(true, io0.isSeekable());
}

//***************************************************************************
// Description: an 8-byte chunk read at offset 0 returns the first two
// uint32 words; at offset 8 returns the next two.
//***************************************************************************

static void test_io_descriptor_seek_read_aligned_chunks()
{
  CDevDescriptor dev;
  CDescObject desc0(g_cfg_descriptor1);
  CIODescriptor io0(desc0);
  io_ddata_t *chunk;
  uint8_t *ptr;

  descriptor_setup_slot0(dev, io0);

  chunk = io0.ddata_alloc(1, 8);
  TEST_ASSERT(chunk != nullptr);
  TEST_ASSERT_EQUAL(8, chunk->getDataSize());

  TEST_ASSERT_EQUAL(OK, io0.getData(*chunk, 1, 0));
  ptr = static_cast<uint8_t *>(chunk->getDataPtr());
  TEST_ASSERT_EQUAL(1, reinterpret_cast<uint32_t *>(ptr)[0]);
  TEST_ASSERT_EQUAL(2, reinterpret_cast<uint32_t *>(ptr)[1]);

  TEST_ASSERT_EQUAL(OK, io0.getData(*chunk, 1, 8));
  ptr = static_cast<uint8_t *>(chunk->getDataPtr());
  TEST_ASSERT_EQUAL(3, reinterpret_cast<uint32_t *>(ptr)[0]);
  TEST_ASSERT_EQUAL(4, reinterpret_cast<uint32_t *>(ptr)[1]);

  free(chunk);
}

//***************************************************************************
// Description: a chunk read at the last offset returns the partial tail
// (4 bytes remaining out of an 8-byte chunk request).
//***************************************************************************

static void test_io_descriptor_seek_read_tail()
{
  CDevDescriptor dev;
  CDescObject desc0(g_cfg_descriptor1);
  CIODescriptor io0(desc0);
  io_ddata_t *chunk;
  uint8_t *ptr;

  descriptor_setup_slot0(dev, io0);

  chunk = io0.ddata_alloc(1, 8);
  TEST_ASSERT(chunk != nullptr);

  TEST_ASSERT_EQUAL(OK, io0.getData(*chunk, 1, 16));
  ptr = static_cast<uint8_t *>(chunk->getDataPtr());
  TEST_ASSERT_EQUAL(5, reinterpret_cast<uint32_t *>(ptr)[0]);

  free(chunk);
}

//***************************************************************************
// Description: a read at the exact end of the descriptor and beyond
// returns -EINVAL.
//***************************************************************************

static void test_io_descriptor_seek_read_past_end()
{
  CDevDescriptor dev;
  CDescObject desc0(g_cfg_descriptor1);
  CIODescriptor io0(desc0);
  io_ddata_t *chunk;

  descriptor_setup_slot0(dev, io0);

  chunk = io0.ddata_alloc(1, 8);
  TEST_ASSERT(chunk != nullptr);

  TEST_ASSERT_EQUAL(-EINVAL, io0.getData(*chunk, 1, 20));
  TEST_ASSERT_EQUAL(-EINVAL, io0.getData(*chunk, 1, 100));

  free(chunk);
}

//***************************************************************************
// Description: a seek read with len != 1 routes to getDataAtImpl which
// returns -EINVAL.
//***************************************************************************

static void test_io_descriptor_seek_read_len_invalid()
{
  CDevDescriptor dev;
  CDescObject desc0(g_cfg_descriptor1);
  CIODescriptor io0(desc0);
  io_ddata_t *chunk;

  descriptor_setup_slot0(dev, io0);

  chunk = io0.ddata_alloc(1, 8);
  TEST_ASSERT(chunk != nullptr);

  TEST_ASSERT_EQUAL(-EINVAL, io0.getData(*chunk, 2, 4));

  free(chunk);
}

//***************************************************************************
// Description: descriptor slot 0 is read-only
//***************************************************************************

static void test_io_descriptor_slot0_readonly()
{
  CDevDescriptor dev;
  CDevDescriptor::SDescriptorReg reg;
  CDescObject desc0(g_cfg_descriptor1);
  CIODescriptor io0(desc0);
  io_ddata_t *chunk;
  int ret;

  reg.ptr = (void *)g_dawn_desc1;
  reg.len = sizeof(g_dawn_desc1);
  ret = dev.regDescriptor(0, reg);
  TEST_ASSERT_EQUAL(OK, ret);

  io0.setDevDescriptor(&dev);
  ret = io0.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  chunk = io0.ddata_alloc(1, 4);
  TEST_ASSERT(chunk != nullptr);
  ((uint8_t *)chunk->getDataPtr())[0] = 0xAA;

  TEST_ASSERT_EQUAL(false, io0.isWrite());
  ret = io0.setData(*chunk, 0);
  TEST_ASSERT_EQUAL(-EROFS, ret);

  free(chunk);
}

#if CONFIG_DAWN_DESC_SLOTS > 1
//***************************************************************************
// Description: descriptor slot 1 supports seeked write
//***************************************************************************

static void test_io_descriptor_slot1_write_seek()
{
  CDevDescriptor dev;
  CDevDescriptor::SDescriptorReg reg;
  CDescObject desc1(g_cfg_descriptor2);
  CIODescriptor io1(desc1);
  io_ddata_t *chunk;
  int ret;
  uint8_t *ptr;

  io1.setDevDescriptor(&dev);
  ret = io1.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  chunk = io1.ddata_alloc(1, 4);
  TEST_ASSERT(chunk != nullptr);
  ptr = (uint8_t *)chunk->getDataPtr();
  ptr[0] = 0x11;
  ptr[1] = 0x22;
  ptr[2] = 0x33;
  ptr[3] = 0x44;

  TEST_ASSERT_EQUAL(true, io1.isWrite());
  ret = io1.setData(*chunk, 3);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = dev.getDescriptor(1, reg);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT(reg.ptr != nullptr);
  TEST_ASSERT_EQUAL(7, reg.len);
  TEST_ASSERT_EQUAL(0x11, ((uint8_t *)reg.ptr)[3]);
  TEST_ASSERT_EQUAL(0x22, ((uint8_t *)reg.ptr)[4]);
  TEST_ASSERT_EQUAL(0x33, ((uint8_t *)reg.ptr)[5]);
  TEST_ASSERT_EQUAL(0x44, ((uint8_t *)reg.ptr)[6]);

  free(chunk);
}

//***************************************************************************
// Description: slot 1 accepts multiple seeked chunks and assembles payload
//***************************************************************************

static void test_io_descriptor_chunked_write()
{
  CDevDescriptor dev;
  CDevDescriptor::SDescriptorReg reg;
  CDescObject desc1(g_cfg_descriptor2);
  CIODescriptor io1(desc1);
  io_ddata_t *chunk;
  int ret;
  uint8_t *ptr;

  io1.setDevDescriptor(&dev);
  ret = io1.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  chunk = io1.ddata_alloc(1, 2);
  TEST_ASSERT(chunk != nullptr);
  ptr = (uint8_t *)chunk->getDataPtr();
  ptr[0] = 0xA1;
  ptr[1] = 0xB2;

  ret = io1.setData(*chunk, 0);
  TEST_ASSERT_EQUAL(OK, ret);

  ptr[0] = 0xC3;
  ptr[1] = 0xD4;

  ret = io1.setData(*chunk, 2);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = dev.getDescriptor(1, reg);
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT(reg.ptr != nullptr);
  TEST_ASSERT_EQUAL(4, reg.len);
  TEST_ASSERT_EQUAL(0xA1, ((uint8_t *)reg.ptr)[0]);
  TEST_ASSERT_EQUAL(0xB2, ((uint8_t *)reg.ptr)[1]);
  TEST_ASSERT_EQUAL(0xC3, ((uint8_t *)reg.ptr)[2]);
  TEST_ASSERT_EQUAL(0xD4, ((uint8_t *)reg.ptr)[3]);

  free(chunk);
}

//***************************************************************************
// Description: slot 1 seeked write beyond slot size returns -ENOSPC
//***************************************************************************

static void test_io_descriptor_write_overflow()
{
  CDevDescriptor dev;
  CDescObject desc1(g_cfg_descriptor2);
  CIODescriptor io1(desc1);
  io_ddata_t *chunk;
  int ret;

  io1.setDevDescriptor(&dev);
  ret = io1.configure();
  TEST_ASSERT_EQUAL(OK, ret);

  chunk = io1.ddata_alloc(1, 8);
  TEST_ASSERT(chunk != nullptr);

  ret = io1.setData(*chunk, CDevDescriptor::SLOT_SIZE - 4);
  TEST_ASSERT_EQUAL(-ENOSPC, ret);

  free(chunk);
}
#endif

extern "C"
{
  int test_io_descriptor()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_descriptor_read_full_slot0);
    DAWN_RUN_TEST(test_io_descriptor_configure_idempotent);
    DAWN_RUN_TEST(test_io_descriptor_alloc_no_chunk_returns_null);
#if CONFIG_DAWN_DESC_SLOTS > 1
    DAWN_RUN_TEST(test_io_descriptor_unregistered_alloc_null);
    DAWN_RUN_TEST(test_io_descriptor_read_full_slot1);
#endif

    DAWN_RUN_TEST(test_io_descriptor_is_seekable);
    DAWN_RUN_TEST(test_io_descriptor_seek_read_aligned_chunks);
    DAWN_RUN_TEST(test_io_descriptor_seek_read_tail);
    DAWN_RUN_TEST(test_io_descriptor_seek_read_past_end);
    DAWN_RUN_TEST(test_io_descriptor_seek_read_len_invalid);

    DAWN_RUN_TEST(test_io_descriptor_slot0_readonly);
#if CONFIG_DAWN_DESC_SLOTS > 1
    DAWN_RUN_TEST(test_io_descriptor_slot1_write_seek);
    DAWN_RUN_TEST(test_io_descriptor_chunked_write);
    DAWN_RUN_TEST(test_io_descriptor_write_overflow);
#endif

    return UNITY_END();
  }
}
