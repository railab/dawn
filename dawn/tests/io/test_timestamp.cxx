// dawn/tests/io/test_timestamp.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/notifier.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/io/timestamp.hxx"
#include "test_common.hxx"

#include <cinttypes>

using namespace dawn;

//***************************************************************************
// Description: timestampIO descriptors
//***************************************************************************

static uint32_t g_cfg_timestamp_fetch_u64[] = {
  CIOTimestamp::objectId(SObjectId::DTYPE_UINT64, false, 0),
  0,
};

static uint32_t g_cfg_timestamp_fetch_u32[] = {
  CIOTimestamp::objectId(SObjectId::DTYPE_UINT32, false, 0),
  0,
};

static uint32_t g_cfg_timestamp_interval_u64[] = {
  CIOTimestamp::objectId(SObjectId::DTYPE_UINT64, false, 0),
  1,
  CIOTimestamp::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_timestamp_interval_u32[] = {
  CIOTimestamp::objectId(SObjectId::DTYPE_UINT32, false, 0),
  1,
  CIOTimestamp::cfgInterval(false),
  5000,
};

// Notifier callback counters

static int g_callback1_cntr = 0;
static int g_callback2_cntr = 0;

static int timestamp_notifier_callback1(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  DAWNASSERT(*(static_cast<uint64_t *>(data->getDataPtr())) > 0, "invalid data");
  printf("TS1 %" PRIu64 "\n", *(static_cast<uint64_t *>(data->getDataPtr())));
  g_callback1_cntr++;
  return OK;
}

static int timestamp_notifier_callback2(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  DAWNASSERT(*(static_cast<uint32_t *>(data->getDataPtr())) > 0, "invalid data");
  printf("TS2 %" PRIu32 "\n", *(static_cast<uint32_t *>(data->getDataPtr())));
  g_callback2_cntr++;
  return OK;
}

//***************************************************************************
// Description: a uint64 fetch-mode timestamp IO returns a non-zero value
// on every getData() call.
//***************************************************************************

static void test_io_timestamp_fetch_uint64()
{
  CDescObject desc(g_cfg_timestamp_fetch_u64);
  CIOTimestamp ts(desc);
  io_sdata_t<uint64_t, 1> data;
  int i;

  TEST_ASSERT_EQUAL(OK, ts.configure());
  TEST_ASSERT_EQUAL(OK, ts.init());
  TEST_ASSERT_EQUAL(OK, ts.start());

  for (i = 0; i < 2; i++)
    {
      TEST_ASSERT_EQUAL(OK, ts.getData(data, 1));
      TEST_ASSERT(data(0) > 0);
    }
}

//***************************************************************************
// Description: a uint32 fetch-mode timestamp IO returns a non-zero value
// on every getData() call.
//***************************************************************************

static void test_io_timestamp_fetch_uint32()
{
  CDescObject desc(g_cfg_timestamp_fetch_u32);
  CIOTimestamp ts(desc);
  io_sdata_t<uint32_t, 1> data;
  int i;

  TEST_ASSERT_EQUAL(OK, ts.configure());
  TEST_ASSERT_EQUAL(OK, ts.init());
  TEST_ASSERT_EQUAL(OK, ts.start());

  for (i = 0; i < 2; i++)
    {
      TEST_ASSERT_EQUAL(OK, ts.getData(data, 1));
      TEST_ASSERT(data(0) > 0);
    }
}

//***************************************************************************
// Description: an interval-mode timestamp IO fires its registered
// notifier callback once per configured interval.
//***************************************************************************

static void test_io_timestamp_interval_fires_callback()
{
  CDescObject desc(g_cfg_timestamp_interval_u64);
  CIOTimestamp ts(desc);
  CIONotifier notifier;

  g_callback1_cntr = 0;

  TEST_ASSERT_EQUAL(OK, ts.configure());
  TEST_ASSERT_EQUAL(OK, ts.init());
  ts.bindNotifier(&notifier);
  TEST_ASSERT_EQUAL(OK, ts.setNotifier(timestamp_notifier_callback1, 0, &ts));

  TEST_ASSERT_EQUAL(OK, ts.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());

  TEST_ASSERT_EQUAL(0, g_callback1_cntr);

  usleep(5000);
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);

  usleep(5000);
  TEST_ASSERT_EQUAL(2, g_callback1_cntr);

  TEST_ASSERT_EQUAL(OK, notifier.stop());
}

//***************************************************************************
// Description: each timestamp IO instance fires its own callback in
// isolation when sharing one notifier.
//***************************************************************************

static void test_io_timestamp_interval_callback_isolation()
{
  CDescObject desc1(g_cfg_timestamp_interval_u64);
  CDescObject desc2(g_cfg_timestamp_interval_u32);
  CIOTimestamp ts1(desc1);
  CIOTimestamp ts2(desc2);
  CIONotifier notifier;

  g_callback1_cntr = 0;
  g_callback2_cntr = 0;

  TEST_ASSERT_EQUAL(OK, ts1.configure());
  TEST_ASSERT_EQUAL(OK, ts1.init());
  TEST_ASSERT_EQUAL(OK, ts2.configure());
  TEST_ASSERT_EQUAL(OK, ts2.init());
  ts1.bindNotifier(&notifier);
  ts2.bindNotifier(&notifier);
  TEST_ASSERT_EQUAL(OK, ts1.setNotifier(timestamp_notifier_callback1, 0, &ts1));
  TEST_ASSERT_EQUAL(OK, ts2.setNotifier(timestamp_notifier_callback2, 0, &ts2));

  TEST_ASSERT_EQUAL(OK, ts1.start());
  TEST_ASSERT_EQUAL(OK, ts2.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());

  usleep(5000);
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);
  TEST_ASSERT_EQUAL(1, g_callback2_cntr);

  TEST_ASSERT_EQUAL(OK, notifier.stop());
}

extern "C"
{
  int test_io_timestamp()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_timestamp_fetch_uint64);
    DAWN_RUN_TEST(test_io_timestamp_fetch_uint32);
    DAWN_RUN_TEST(test_io_timestamp_interval_fires_callback);
    DAWN_RUN_TEST(test_io_timestamp_interval_callback_isolation);

    return UNITY_END();
  }
}
