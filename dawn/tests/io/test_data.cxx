// dawn/tests/io/test_data.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/ddata.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/io/viewdata.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: io_sdata_t reports the right type size, item count, batch
// size, and total data size for typical instantiations.
//***************************************************************************

static void test_io_sdata_metadata()
{
  io_sdata_t<uint32_t, 10, 1, true> data1;
  io_sdata_t<float, 5, 1, true> data2;
  io_sdata_t<uint64_t, 3, 4, true> data3;

  TEST_ASSERT_EQUAL(sizeof(uint32_t), data1.getSize());
  TEST_ASSERT_EQUAL(sizeof(float), data2.getSize());
  TEST_ASSERT_EQUAL(sizeof(uint64_t), data3.getSize());

  TEST_ASSERT_EQUAL(10, data1.getItems());
  TEST_ASSERT_EQUAL(5, data2.getItems());
  TEST_ASSERT_EQUAL(3, data3.getItems());

  TEST_ASSERT_EQUAL(1, data1.getBatch());
  TEST_ASSERT_EQUAL(1, data2.getBatch());
  TEST_ASSERT_EQUAL(4, data3.getBatch());

  TEST_ASSERT_EQUAL(sizeof(uint32_t) * 10, data1.getDataSize());
  TEST_ASSERT_EQUAL(sizeof(float) * 5, data2.getDataSize());
  TEST_ASSERT_EQUAL(sizeof(uint64_t) * 3, data3.getDataSize());
}

//***************************************************************************
// Description: a freshly constructed io_sdata_t reads back zero for every
// element across all batches.
//***************************************************************************

static void test_io_sdata_default_zero()
{
  io_sdata_t<uint32_t, 10, 1, true> data1;
  io_sdata_t<uint64_t, 3, 4, true> data3;
  size_t i;
  size_t b;

  for (i = 0; i < 10; i++)
    {
      TEST_ASSERT_EQUAL(0, data1(i));
    }
  TEST_ASSERT_EQUAL(0, data1[0]);

  for (b = 0; b < 4; b++)
    {
      for (i = 0; i < 3; i++)
        {
          TEST_ASSERT_EQUAL(0, data3(i, b));
        }
      TEST_ASSERT_EQUAL(0, data3[b]);
    }
}

//***************************************************************************
// Description: writing through operator() updates the underlying storage
// and the batched-index variant works for multi-batch storage.
//***************************************************************************

static void test_io_sdata_modify_values()
{
  io_sdata_t<uint32_t, 10, 1, true> data1;
  io_sdata_t<uint64_t, 3, 4, true> data3;

  data1(0) = true;
  data1(1) = false;
  data1(2) = true;
  data1(3) = false;
  TEST_ASSERT_EQUAL(true, data1(0));
  TEST_ASSERT_EQUAL(false, data1(1));
  TEST_ASSERT_EQUAL(true, data1(2));
  TEST_ASSERT_EQUAL(false, data1(3));

  data3(0, 0) = 12;
  data3(0, 1) = 34;
  data3(1, 0) = 56;
  data3(1, 1) = 78;
  data3(2, 0) = 910;
  data3(2, 1) = 1112;
  TEST_ASSERT_EQUAL(12, data3(0, 0));
  TEST_ASSERT_EQUAL(34, data3(0, 1));
  TEST_ASSERT_EQUAL(56, data3(1, 0));
  TEST_ASSERT_EQUAL(78, data3(1, 1));
  TEST_ASSERT_EQUAL(910, data3(2, 0));
  TEST_ASSERT_EQUAL(1112, data3(2, 1));
}

//***************************************************************************
// Description: io_sdata_t timestamps are settable per batch and the
// indexed accessor reflects the same value.
//***************************************************************************

static void test_io_sdata_timestamps()
{
  io_sdata_t<uint32_t, 10, 1, true> data1;
  io_sdata_t<uint64_t, 3, 4, true> data3;

  data1.getTs() = 12345;
  TEST_ASSERT_EQUAL(12345, data1.getTs());
  TEST_ASSERT_EQUAL(12345, data1[0]);

  data3.getTs(0) = 123;
  data3.getTs(1) = 456;
  data3.getTs(2) = 789;
  TEST_ASSERT_EQUAL(123, data3.getTs(0));
  TEST_ASSERT_EQUAL(456, data3.getTs(1));
  TEST_ASSERT_EQUAL(789, data3.getTs(2));
  TEST_ASSERT_EQUAL(123, data3[0]);
  TEST_ASSERT_EQUAL(456, data3[1]);
  TEST_ASSERT_EQUAL(789, data3[2]);
}

//***************************************************************************
// Description: io_ddata_t reports the right metadata for typical
// dynamic instantiations.
//***************************************************************************

static void test_io_ddata_metadata()
{
  io_ddata_t data1(sizeof(uint32_t), 10, 1, 0, true);
  io_ddata_t data2(sizeof(float), 5, 1, 0, true);
  io_ddata_t data3(sizeof(uint64_t), 3, 4, 0, true);
  io_ddata_t data4(sizeof(uint32_t), 1, 4, 0, false);
  uint8_t *ptr;

  TEST_ASSERT_EQUAL(sizeof(uint32_t), data1.getSize());
  TEST_ASSERT_EQUAL(sizeof(float), data2.getSize());
  TEST_ASSERT_EQUAL(sizeof(uint64_t), data3.getSize());

  TEST_ASSERT_EQUAL(10, data1.getItems());
  TEST_ASSERT_EQUAL(5, data2.getItems());
  TEST_ASSERT_EQUAL(3, data3.getItems());

  TEST_ASSERT_EQUAL(1, data1.getBatch());
  TEST_ASSERT_EQUAL(1, data2.getBatch());
  TEST_ASSERT_EQUAL(4, data3.getBatch());

  TEST_ASSERT_EQUAL(sizeof(uint32_t) * 10, data1.getDataSize());
  TEST_ASSERT_EQUAL(sizeof(float) * 5, data2.getDataSize());
  TEST_ASSERT_EQUAL(sizeof(uint64_t) * 3, data3.getDataSize());

  TEST_ASSERT_FALSE(data4.hasTimestamp());
  TEST_ASSERT_EQUAL(sizeof(uint32_t), data4.getDataSize());
  TEST_ASSERT_EQUAL(sizeof(uint32_t) * 4, data4.getBufferSize());
  ptr = static_cast<uint8_t *>(data4.getDataPtr());
  TEST_ASSERT_EQUAL_PTR(ptr + sizeof(uint32_t), data4.getDataPtr(1));
}

//***************************************************************************
// Description: a freshly constructed io_ddata_t reads back zero for every
// element across all batches.
//***************************************************************************

static void test_io_ddata_default_zero()
{
  io_ddata_t data1(sizeof(uint32_t), 10, 1, 0, true);
  io_ddata_t data3(sizeof(uint64_t), 3, 4, 0, true);
  size_t i;
  size_t b;

  for (i = 0; i < 10; i++)
    {
      TEST_ASSERT_EQUAL(0, data1.get<uint32_t>(i));
    }
  TEST_ASSERT_EQUAL(0, data1[0]);

  for (b = 0; b < 4; b++)
    {
      for (i = 0; i < 3; i++)
        {
          TEST_ASSERT_EQUAL(0, data3.get<uint32_t>(i, b));
        }
      TEST_ASSERT_EQUAL(0, data3[b]);
    }
}

//***************************************************************************
// Description: writing through io_ddata_t::get<T>() updates the underlying
// storage and round-trips via the same accessor.
//***************************************************************************

static void test_io_ddata_modify_values()
{
  io_ddata_t data1(sizeof(uint32_t), 10, 1, 0, true);
  io_ddata_t data3(sizeof(uint64_t), 3, 4, 0, true);

  data1.get<uint32_t>(0) = true;
  data1.get<uint32_t>(1) = false;
  data1.get<uint32_t>(2) = true;
  data1.get<uint32_t>(3) = false;
  TEST_ASSERT_EQUAL(true, data1.get<uint32_t>(0));
  TEST_ASSERT_EQUAL(false, data1.get<uint32_t>(1));
  TEST_ASSERT_EQUAL(true, data1.get<uint32_t>(2));
  TEST_ASSERT_EQUAL(false, data1.get<uint32_t>(3));

  data3.get<uint64_t>(0, 0) = 12;
  data3.get<uint64_t>(0, 1) = 34;
  data3.get<uint64_t>(1, 0) = 56;
  data3.get<uint64_t>(1, 1) = 78;
  data3.get<uint64_t>(2, 0) = 910;
  data3.get<uint64_t>(2, 1) = 1112;
  TEST_ASSERT_EQUAL(12, data3.get<uint64_t>(0, 0));
  TEST_ASSERT_EQUAL(34, data3.get<uint64_t>(0, 1));
  TEST_ASSERT_EQUAL(56, data3.get<uint64_t>(1, 0));
  TEST_ASSERT_EQUAL(78, data3.get<uint64_t>(1, 1));
  TEST_ASSERT_EQUAL(910, data3.get<uint64_t>(2, 0));
  TEST_ASSERT_EQUAL(1112, data3.get<uint64_t>(2, 1));
}

//***************************************************************************
// Description: io_ddata_t timestamps are settable per batch.
//***************************************************************************

static void test_io_ddata_timestamps()
{
  io_ddata_t data1(sizeof(uint32_t), 10, 1, 0, true);
  io_ddata_t data3(sizeof(uint64_t), 3, 4, 0, true);

  data1.getTs() = 12345;
  TEST_ASSERT_EQUAL(12345, data1.getTs());
  TEST_ASSERT_EQUAL(12345, data1[0]);

  data3.getTs(0) = 123;
  data3.getTs(1) = 456;
  data3.getTs(2) = 789;
  TEST_ASSERT_EQUAL(123, data3.getTs(0));
  TEST_ASSERT_EQUAL(456, data3.getTs(1));
  TEST_ASSERT_EQUAL(789, data3.getTs(2));
  TEST_ASSERT_EQUAL(123, data3[0]);
  TEST_ASSERT_EQUAL(456, data3[1]);
  TEST_ASSERT_EQUAL(789, data3[2]);
}

//***************************************************************************
// Description: io_data_view_t reports metadata for caller-owned storage and
// exposes that storage through the IODataCmn pointer accessors.
//***************************************************************************

static void test_io_data_view_metadata()
{
  uint8_t buffer[6] = {};
  io_data_view_t data(buffer, sizeof(buffer), 3);

  TEST_ASSERT_EQUAL(3, data.getItems());
  TEST_ASSERT_EQUAL(sizeof(buffer), data.getDataSize());
  TEST_ASSERT_FALSE(data.hasTimestamp());
  TEST_ASSERT_EQUAL_PTR(buffer, data.getPtr());
  TEST_ASSERT_EQUAL_PTR(buffer, data.getDataPtr());
  TEST_ASSERT_EQUAL_PTR(buffer, data.getPtr(4));
  TEST_ASSERT_EQUAL_PTR(buffer, data.getDataPtr(4));
}

//***************************************************************************
// Description: io_data_view_t is non-owning; writes through the data pointer
// update the caller-provided backing buffer.
//***************************************************************************

static void test_io_data_view_backing_storage()
{
  uint8_t buffer[4] = {1, 2, 3, 4};
  io_data_view_t data(buffer, sizeof(buffer));
  uint8_t *ptr = static_cast<uint8_t *>(data.getDataPtr());

  ptr[1] = 0xaa;
  ptr[3] = 0x55;

  TEST_ASSERT_EQUAL(1, buffer[0]);
  TEST_ASSERT_EQUAL(0xaa, buffer[1]);
  TEST_ASSERT_EQUAL(3, buffer[2]);
  TEST_ASSERT_EQUAL(0x55, buffer[3]);
}

//***************************************************************************
// Description: io_data_view_t has no timestamp storage and returns a shared
// dummy timestamp reference for API compatibility.
//***************************************************************************

static void test_io_data_view_timestamp_dummy()
{
  uint8_t buffer[2] = {};
  io_data_view_t data(buffer, sizeof(buffer));

  data.getTs() = 12345;

  TEST_ASSERT_FALSE(data.hasTimestamp());
  TEST_ASSERT_EQUAL(12345, data.getTs());
  TEST_ASSERT_EQUAL(12345, data.getTs(7));
}

//***************************************************************************
// Description: an sdata and ddata with matching shape produce identical
// raw buffers when populated with the same uint32 payload.
//***************************************************************************

static void test_io_data_compat_uint32()
{
  io_sdata_t<uint32_t, 10, 2, true> sdata;
  io_ddata_t ddata(sizeof(uint32_t), 10, 2, 0, true);
  uint8_t *ptr1;
  uint8_t *ptr2;
  size_t i;

  TEST_ASSERT_EQUAL(sdata.getBufferSize(), ddata.getBufferSize());

  sdata(0, 0) = true;
  sdata(1, 0) = false;
  sdata(2, 0) = true;
  sdata(3, 0) = false;
  sdata[0] = 0xdeadbeef;
  sdata(0, 1) = true;
  sdata(1, 1) = false;
  sdata(2, 1) = true;
  sdata(3, 1) = false;
  sdata[1] = 0xdeadbeef;

  ddata.get<uint32_t>(0, 0) = true;
  ddata.get<uint32_t>(1, 0) = false;
  ddata.get<uint32_t>(2, 0) = true;
  ddata.get<uint32_t>(3, 0) = false;
  ddata[0] = 0xdeadbeef;
  ddata.get<uint32_t>(0, 1) = true;
  ddata.get<uint32_t>(1, 1) = false;
  ddata.get<uint32_t>(2, 1) = true;
  ddata.get<uint32_t>(3, 1) = false;
  ddata[1] = 0xdeadbeef;

  ptr1 = static_cast<uint8_t *>(sdata.getPtr(0));
  ptr2 = static_cast<uint8_t *>(ddata.getPtr(0));
  for (i = 0; i < sdata.getBufferSize(); i++)
    {
      TEST_ASSERT_EQUAL(ptr1[i], ptr2[i]);
    }
}

//***************************************************************************
// Description: an sdata and ddata with matching shape produce identical
// raw buffers when populated with the same float payload.
//***************************************************************************

static void test_io_data_compat_float()
{
  io_sdata_t<float, 5, 2, true> sdata;
  io_ddata_t ddata(sizeof(float), 5, 2, 0, true);
  uint8_t *ptr1;
  uint8_t *ptr2;
  size_t i;

  TEST_ASSERT_EQUAL(sdata.getBufferSize(), ddata.getBufferSize());

  sdata(0, 0) = 1.0f;
  sdata(1, 0) = 0.0f;
  sdata(2, 0) = -10.0f;
  sdata(3, 0) = 125.0f;
  sdata[0] = 0xdeadbeef;
  sdata(0, 1) = 2.0f;
  sdata(1, 1) = 0.1f;
  sdata(2, 1) = -1.0f;
  sdata(3, 1) = -125.0f;
  sdata[1] = 0xdeadbeef;

  ddata.get<float>(0, 0) = 1.0f;
  ddata.get<float>(1, 0) = 0.0f;
  ddata.get<float>(2, 0) = -10.0f;
  ddata.get<float>(3, 0) = 125.0f;
  ddata[0] = 0xdeadbeef;
  ddata.get<float>(0, 1) = 2.0f;
  ddata.get<float>(1, 1) = 0.1f;
  ddata.get<float>(2, 1) = -1.0f;
  ddata.get<float>(3, 1) = -125.0f;
  ddata[1] = 0xdeadbeef;

  ptr1 = static_cast<uint8_t *>(sdata.getPtr(0));
  ptr2 = static_cast<uint8_t *>(ddata.getPtr(0));
  for (i = 0; i < sdata.getBufferSize(); i++)
    {
      TEST_ASSERT_EQUAL(ptr1[i], ptr2[i]);
    }
}

//***************************************************************************
// Description: an sdata and ddata with matching shape produce identical
// raw buffers when populated with the same uint64 payload.
//***************************************************************************

static void test_io_data_compat_uint64()
{
  io_sdata_t<uint64_t, 3, 4, true> sdata;
  io_ddata_t ddata(sizeof(uint64_t), 3, 4, 0, true);
  uint8_t *ptr1;
  uint8_t *ptr2;
  size_t i;

  TEST_ASSERT_EQUAL(sdata.getBufferSize(), ddata.getBufferSize());

  sdata(0, 0) = 111;
  sdata(1, 0) = 222;
  sdata(2, 0) = 333;
  sdata[0] = 0xdeadbeef;
  sdata(0, 1) = 11111111;
  sdata(1, 1) = 22222222;
  sdata(2, 1) = 59999999999;
  sdata[1] = 0xdeadbeef;

  ddata.get<uint64_t>(0, 0) = 111;
  ddata.get<uint64_t>(1, 0) = 222;
  ddata.get<uint64_t>(2, 0) = 333;
  ddata[0] = 0xdeadbeef;
  ddata.get<uint64_t>(0, 1) = 11111111;
  ddata.get<uint64_t>(1, 1) = 22222222;
  ddata.get<uint64_t>(2, 1) = 59999999999;
  ddata[1] = 0xdeadbeef;

  ptr1 = static_cast<uint8_t *>(sdata.getPtr(0));
  ptr2 = static_cast<uint8_t *>(ddata.getPtr(0));
  for (i = 0; i < sdata.getBufferSize(); i++)
    {
      TEST_ASSERT_EQUAL(ptr1[i], ptr2[i]);
    }
}

extern "C"
{
  int test_io_data()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_sdata_metadata);
    DAWN_RUN_TEST(test_io_sdata_default_zero);
    DAWN_RUN_TEST(test_io_sdata_modify_values);
    DAWN_RUN_TEST(test_io_sdata_timestamps);

    DAWN_RUN_TEST(test_io_ddata_metadata);
    DAWN_RUN_TEST(test_io_ddata_default_zero);
    DAWN_RUN_TEST(test_io_ddata_modify_values);
    DAWN_RUN_TEST(test_io_ddata_timestamps);

    DAWN_RUN_TEST(test_io_data_view_metadata);
    DAWN_RUN_TEST(test_io_data_view_backing_storage);
    DAWN_RUN_TEST(test_io_data_view_timestamp_dummy);

    DAWN_RUN_TEST(test_io_data_compat_uint32);
    DAWN_RUN_TEST(test_io_data_compat_float);
    DAWN_RUN_TEST(test_io_data_compat_uint64);

    return UNITY_END();
  }
}
