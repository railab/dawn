// dawn/tests/io/test_systime.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/systime.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: System time IO configuration
//***************************************************************************

static uint32_t g_cfg_systime[] = {
  CIOSystime::objectId(0),
  0,
};

//***************************************************************************
// Description: systime IO is read+write, single-dim, non-batch, non-notify.
//***************************************************************************

static void test_io_systime_properties()
{
  CDescObject desc(g_cfg_systime);
  CIOSystime systime(desc);
  io_sdata_t<uint64_t, 1> data;

  TEST_ASSERT_EQUAL(OK, systime.configure());
  TEST_ASSERT_EQUAL(OK, systime.init());

  TEST_ASSERT_EQUAL(data.getDataSize(), systime.getDataSize());
  TEST_ASSERT_EQUAL(1, systime.getDataDim());
  TEST_ASSERT_TRUE(systime.isRead());
  TEST_ASSERT_TRUE(systime.isWrite());
  TEST_ASSERT_FALSE(systime.isNotify());
  TEST_ASSERT_FALSE(systime.isBatch());
}

//***************************************************************************
// Description: a batched read returns -ENOTSUP.
//***************************************************************************

static void test_io_systime_batch_unsupported()
{
  CDescObject desc(g_cfg_systime);
  CIOSystime systime(desc);
  io_sdata_t<uint64_t, 1> data;

  TEST_ASSERT_EQUAL(OK, systime.configure());
  TEST_ASSERT_EQUAL(OK, systime.init());

  TEST_ASSERT_EQUAL(-ENOTSUP, systime.getData(data, 2));
}

//***************************************************************************
// Description: a fresh read returns a positive system time value.
//***************************************************************************

static void test_io_systime_read_positive()
{
  CDescObject desc(g_cfg_systime);
  CIOSystime systime(desc);
  io_sdata_t<uint64_t, 1> data;

  TEST_ASSERT_EQUAL(OK, systime.configure());
  TEST_ASSERT_EQUAL(OK, systime.init());

  TEST_ASSERT_EQUAL(OK, systime.getData(data, 1));
  TEST_ASSERT(data(0) > 0);
}

//***************************************************************************
// Description: setData() updates the system clock; the next read falls
// within ~1 second of the seeded value.
//***************************************************************************

static void test_io_systime_set_then_read()
{
  CDescObject desc(g_cfg_systime);
  CIOSystime systime(desc);
  io_sdata_t<uint64_t, 1> data;

  TEST_ASSERT_EQUAL(OK, systime.configure());
  TEST_ASSERT_EQUAL(OK, systime.init());

  data(0) = 1577836800ULL * 1000000000ULL; // year 2020 in ns
  TEST_ASSERT_EQUAL(OK, systime.setData(data));

  TEST_ASSERT_EQUAL(OK, systime.getData(data, 1));
  TEST_ASSERT(data(0) >= 1577836800ULL * 1000000000ULL);
  TEST_ASSERT(data(0) < (1577836800ULL + 1ULL) * 1000000000ULL);
}

//***************************************************************************
// Description: after a known seed, time advances forward as wall-clock
// time passes.
//***************************************************************************

static void test_io_systime_advances()
{
  CDescObject desc(g_cfg_systime);
  CIOSystime systime(desc);
  io_sdata_t<uint64_t, 1> data;

  TEST_ASSERT_EQUAL(OK, systime.configure());
  TEST_ASSERT_EQUAL(OK, systime.init());

  data(0) = 1577836800ULL * 1000000000ULL;
  TEST_ASSERT_EQUAL(OK, systime.setData(data));

  usleep(100000);

  TEST_ASSERT_EQUAL(OK, systime.getData(data, 1));
  TEST_ASSERT(data(0) >= (1577836800ULL * 1000000000ULL + 80000000ULL));
  TEST_ASSERT(data(0) < (1577836800ULL * 1000000000ULL + 200000000ULL));
}

extern "C"
{
  int test_io_system_systime()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_systime_properties);
    DAWN_RUN_TEST(test_io_systime_batch_unsupported);
    DAWN_RUN_TEST(test_io_systime_read_positive);
    DAWN_RUN_TEST(test_io_systime_set_then_read);
    DAWN_RUN_TEST(test_io_systime_advances);

    return UNITY_END();
  }
}
