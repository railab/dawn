// dawn/tests/io/test_rand.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/notifier.hxx"
#include "dawn/io/rand.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_rand_fetch_u64[] = {
  CIORand::objectId(SObjectId::DTYPE_UINT64, false, 0),
  0,
};

static uint32_t g_cfg_rand_fetch_u32[] = {
  CIORand::objectId(SObjectId::DTYPE_UINT32, false, 0),
  0,
};

static uint32_t g_cfg_rand_interval_u64[] = {
  CIORand::objectId(SObjectId::DTYPE_UINT64, false, 0),
  1,
  CIORand::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_rand_interval_u8[] = {
  CIORand::objectId(SObjectId::DTYPE_UINT8, false, 0),
  1,
  CIORand::cfgInterval(false),
  5000,
};

// Notifier callback counters

static int g_callback1_cntr = 0;
static int g_callback2_cntr = 0;

static int rand_notifier_callback1(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  printf("RAND1 %" PRIx64 "\n", *(static_cast<uint64_t *>(data->getDataPtr())));
  g_callback1_cntr++;
  return OK;
}

static int rand_notifier_callback2(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  printf("RAND2 %" PRIx8 "\n", *(static_cast<uint8_t *>(data->getDataPtr())));
  g_callback2_cntr++;
  return OK;
}

//***************************************************************************
// Description: a uint64 fetch-mode rand IO returns a non-zero value on
// every getData() call.
//***************************************************************************

static void test_io_rand_fetch_uint64()
{
  CDescObject desc(g_cfg_rand_fetch_u64);
  CIORand rng(desc);
  io_sdata_t<uint64_t, 1> data;
  int i;

  TEST_ASSERT_EQUAL(OK, rng.configure());
  TEST_ASSERT_EQUAL(OK, rng.init());
  TEST_ASSERT_EQUAL(OK, rng.start());

  for (i = 0; i < 2; i++)
    {
      TEST_ASSERT_EQUAL(OK, rng.getData(data, 1));
      TEST_ASSERT(data(0) > 0);
    }
}

//***************************************************************************
// Description: a uint32 fetch-mode rand IO returns a non-zero value on
// every getData() call.
//***************************************************************************

static void test_io_rand_fetch_uint32()
{
  CDescObject desc(g_cfg_rand_fetch_u32);
  CIORand rng(desc);
  io_sdata_t<uint32_t, 1> data;
  int i;

  TEST_ASSERT_EQUAL(OK, rng.configure());
  TEST_ASSERT_EQUAL(OK, rng.init());
  TEST_ASSERT_EQUAL(OK, rng.start());

  for (i = 0; i < 2; i++)
    {
      TEST_ASSERT_EQUAL(OK, rng.getData(data, 1));
      TEST_ASSERT(data(0) > 0);
    }
}

//***************************************************************************
// Description: an interval-mode rand IO fires its registered notifier
// callback once per configured interval.
//***************************************************************************

static void test_io_rand_interval_fires_callback()
{
  CDescObject desc(g_cfg_rand_interval_u64);
  CIORand rng(desc);
  CIONotifier notifier;

  g_callback1_cntr = 0;

  TEST_ASSERT_EQUAL(OK, rng.configure());
  TEST_ASSERT_EQUAL(OK, rng.init());
  rng.bindNotifier(&notifier);
  TEST_ASSERT_EQUAL(OK, rng.setNotifier(rand_notifier_callback1, 0, &rng));

  TEST_ASSERT_EQUAL(OK, rng.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());

  TEST_ASSERT_EQUAL(0, g_callback1_cntr);

  usleep(5000);
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);

  usleep(5000);
  TEST_ASSERT_EQUAL(2, g_callback1_cntr);

  TEST_ASSERT_EQUAL(OK, notifier.stop());
}

//***************************************************************************
// Description: each rand IO instance fires its own callback in isolation
// (callbacks are registered per IO and don't cross over).
//***************************************************************************

static void test_io_rand_interval_callback_isolation()
{
  CDescObject desc1(g_cfg_rand_interval_u64);
  CDescObject desc2(g_cfg_rand_interval_u8);
  CIORand rng1(desc1);
  CIORand rng2(desc2);
  CIONotifier notifier;

  g_callback1_cntr = 0;
  g_callback2_cntr = 0;

  TEST_ASSERT_EQUAL(OK, rng1.configure());
  TEST_ASSERT_EQUAL(OK, rng1.init());
  TEST_ASSERT_EQUAL(OK, rng2.configure());
  TEST_ASSERT_EQUAL(OK, rng2.init());
  rng1.bindNotifier(&notifier);
  rng2.bindNotifier(&notifier);
  TEST_ASSERT_EQUAL(OK, rng1.setNotifier(rand_notifier_callback1, 0, &rng1));
  TEST_ASSERT_EQUAL(OK, rng2.setNotifier(rand_notifier_callback2, 0, &rng2));

  TEST_ASSERT_EQUAL(OK, rng1.start());
  TEST_ASSERT_EQUAL(OK, rng2.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());

  usleep(5000);
  TEST_ASSERT_EQUAL(1, g_callback1_cntr);
  TEST_ASSERT_EQUAL(1, g_callback2_cntr);

  TEST_ASSERT_EQUAL(OK, notifier.stop());
}

extern "C"
{
  int test_io_rand()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_rand_fetch_uint64);
    DAWN_RUN_TEST(test_io_rand_fetch_uint32);
    DAWN_RUN_TEST(test_io_rand_interval_fires_callback);
    DAWN_RUN_TEST(test_io_rand_interval_callback_isolation);

    return UNITY_END();
  }
}
