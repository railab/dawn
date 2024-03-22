// dawn/tests/io/test_dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dummy.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: test descriptors for DUMMY
//***************************************************************************

static uint32_t g_cfg_dummy[] = {
  CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 0),
  0,
};

static uint32_t g_cfg_dummy_vec[] = {
  CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 1),
  2,
  CIODummy::cfgIdDim(),
  4,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 4),
  1,
  2,
  3,
  4,
};

static uint32_t g_cfg_dummy_u64[] = {
  CIODummy::objectId(SObjectId::DTYPE_UINT64, false, 2),
  2,
  CIODummy::cfgIdDim(),
  2,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT64, true, 2),
  0x89abcdef,
  0x01234567,
  0x76543210,
  0xfedcba98,
};

//***************************************************************************
// Description: check DUMMY implementation
//***************************************************************************

static void test_io_dummy_init()
{
  CDescObject desc(g_cfg_dummy);
  CIODummy dummy(desc);
  io_sdata_t<uint32_t, 1> data;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());

  // Tests

  TEST_ASSERT(dummy.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(data(0), 0);
  TEST_ASSERT_EQUAL(data[0], 0);

  data(0) = 0xdeadbeef;
  TEST_ASSERT(dummy.setData(data) == OK);

  TEST_ASSERT(dummy.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(data(0), 0xdeadbeef);
  TEST_ASSERT_EQUAL(data[0], 0);

  data(0) = 0xffffffff;
  TEST_ASSERT(dummy.setData(data) == OK);

  TEST_ASSERT(dummy.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(data(0), 0xffffffff);
  TEST_ASSERT_EQUAL(data[0], 0);
}

//***************************************************************************
// Description: multidimensional dummy IO preserves all configured elements.
//***************************************************************************

static void test_io_dummy_multidim()
{
  CDescObject desc(g_cfg_dummy_vec);
  CIODummy dummy(desc);
  io_sdata_t<uint32_t, 4> data;

  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());

  TEST_ASSERT_EQUAL(4, dummy.getDataDim());
  TEST_ASSERT_EQUAL(sizeof(uint32_t) * 4, dummy.getDataSize());

  TEST_ASSERT(dummy.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(1, data(0, 0));
  TEST_ASSERT_EQUAL(2, data(1, 0));
  TEST_ASSERT_EQUAL(3, data(2, 0));
  TEST_ASSERT_EQUAL(4, data(3, 0));

  data(0, 0) = 10;
  data(1, 0) = 11;
  data(2, 0) = 12;
  data(3, 0) = 13;

  TEST_ASSERT(dummy.setData(data) == OK);
  TEST_ASSERT(dummy.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL(10, data(0, 0));
  TEST_ASSERT_EQUAL(11, data(1, 0));
  TEST_ASSERT_EQUAL(12, data(2, 0));
  TEST_ASSERT_EQUAL(13, data(3, 0));
}

//***************************************************************************
// Description: uint64 dummy IO reads back configured 64-bit values.
//***************************************************************************

static void test_io_dummy_init_u64()
{
  CDescObject desc(g_cfg_dummy_u64);
  CIODummy dummy(desc);
  io_sdata_t<uint64_t, 2> data;

  TEST_ASSERT_EQUAL(OK, dummy.configure());
  TEST_ASSERT_EQUAL(OK, dummy.init());

  TEST_ASSERT_EQUAL(2, dummy.getDataDim());
  TEST_ASSERT_EQUAL(sizeof(uint64_t) * 2, dummy.getDataSize());

  TEST_ASSERT(dummy.getData(data, 1) == OK);
  TEST_ASSERT_EQUAL_UINT64(0x0123456789abcdefULL, data(0, 0));
  TEST_ASSERT_EQUAL_UINT64(0xfedcba9876543210ULL, data(1, 0));
}

extern "C"
{
  int test_io_dummy()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_dummy_init);
    DAWN_RUN_TEST(test_io_dummy_multidim);
    DAWN_RUN_TEST(test_io_dummy_init_u64);

    return UNITY_END();
  }
}
