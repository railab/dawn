// dawn/tests/io/test_dummy_notify.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_dummy_notify[] = {
  CIODummyNotify::objectId(SObjectId::DTYPE_UINT32, false, 0),
  3,
  CIODummyNotify::cfgIdDim(),
  3,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 3),
  7,
  8,
  9,
  CIODummyNotify::cfgInterval(false),
  20000,
};

static uint32_t g_cfg_dummy_notify_u64[] = {
  CIODummyNotify::objectId(SObjectId::DTYPE_UINT64, false, 1),
  3,
  CIODummyNotify::cfgIdDim(),
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT64, true, 2),
  0x89abcdef,
  0x01234567,
  0x76543210,
  0xfedcba98,
  CIODummyNotify::cfgInterval(false),
  20000,
};

//***************************************************************************
// Description: notifying dummy IO starts, updates, and reads multidim data.
//***************************************************************************

static void test_io_dummy_notify_init()
{
  CDescObject desc(g_cfg_dummy_notify);
  CIODummyNotify dummy(desc);
  io_sdata_t<uint32_t, 3> data;

  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  TEST_ASSERT_EQUAL(OK, dummy.start());

  TEST_ASSERT_EQUAL(3, dummy.getDataDim());
  TEST_ASSERT_EQUAL(sizeof(uint32_t) * 3, dummy.getDataSize());

  usleep(25000);
  TEST_ASSERT(dummy.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(7, data(0, 0));
  TEST_ASSERT_EQUAL(8, data(1, 0));
  TEST_ASSERT_EQUAL(9, data(2, 0));

  data(0, 0) = 10;
  data(1, 0) = 11;
  data(2, 0) = 12;

  TEST_ASSERT(dummy.setData(data) == OK);
  usleep(25000);
  TEST_ASSERT(dummy.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(10, data(0, 0));
  TEST_ASSERT_EQUAL(11, data(1, 0));
  TEST_ASSERT_EQUAL(12, data(2, 0));
  TEST_ASSERT_EQUAL(OK, dummy.stop());
}

//***************************************************************************
// Description: notifying uint64 dummy IO reads configured 64-bit values.
//***************************************************************************

static void test_io_dummy_notify_init_u64()
{
  CDescObject desc(g_cfg_dummy_notify_u64);
  CIODummyNotify dummy(desc);
  io_sdata_t<uint64_t, 2> data;

  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());
  TEST_ASSERT_EQUAL(OK, dummy.start());

  usleep(25000);
  TEST_ASSERT(dummy.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL_UINT64(0x0123456789abcdefULL, data(0, 0));
  TEST_ASSERT_EQUAL_UINT64(0xfedcba9876543210ULL, data(1, 0));
  TEST_ASSERT_EQUAL(OK, dummy.stop());
}

extern "C"
{
  int test_io_dummy_notify()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_dummy_notify_init);
    DAWN_RUN_TEST(test_io_dummy_notify_init_u64);

    return UNITY_END();
  }
}
