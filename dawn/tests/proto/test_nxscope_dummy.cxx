// dawn/tests/proto/test_nxscope_dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/proto/nxscope/dummy.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto NXSCOPE_DUMMYIO1 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto NXSCOPE_DUMMYIO2 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto NXSCOPE_DUMMYIO3 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 3);

static uint32_t g_cfg_dummy1[] = {
  NXSCOPE_DUMMYIO1,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_dummy2[] = {
  NXSCOPE_DUMMYIO2,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_dummy3[] = {
  NXSCOPE_DUMMYIO3,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_bin_nxscope_dummy[] = {
  CProtoNxscopeDummy::objectId(0),
  1,
  CProtoNxscopeDummy::cfgIdIOBind2(3),
  NXSCOPE_DUMMYIO1,
  0x00000061,
  0x00000000,
  0x00000000,
  NXSCOPE_DUMMYIO2,
  0x00000062,
  0x00000000,
  0x00000000,
  NXSCOPE_DUMMYIO3,
  0x00000063,
  0x00000000,
  0x00000000,
};

// Configure + init three CIODummyNotify dummies, configure + bind the
// nxscope dummy proto, init + start.  Caller stops nxscope and notifier.

#define NXSCOPE_DUMMY_FIXTURE                          \
  CDescObject descv1(g_cfg_dummy1);                    \
  CIODummyNotify dummy1(descv1);                       \
  CDescObject descv2(g_cfg_dummy2);                    \
  CIODummyNotify dummy2(descv2);                       \
  CDescObject descv3(g_cfg_dummy3);                    \
  CIODummyNotify dummy3(descv3);                       \
  CDescObject desc(g_bin_nxscope_dummy);               \
  CProtoNxscopeDummy nxscope(desc);                    \
  CIONotifier notifier;                                \
  TEST_ASSERT_EQUAL(OK, nxscope.configure());          \
  TEST_ASSERT_EQUAL(OK, dummy1.configure());           \
  TEST_ASSERT_EQUAL(OK, dummy1.init());                \
  TEST_ASSERT_EQUAL(OK, dummy2.configure());           \
  TEST_ASSERT_EQUAL(OK, dummy2.init());                \
  TEST_ASSERT_EQUAL(OK, dummy3.configure());           \
  TEST_ASSERT_EQUAL(OK, dummy3.init());                \
  dummy1.bindNotifier(&notifier);                      \
  dummy2.bindNotifier(&notifier);                      \
  dummy3.bindNotifier(&notifier);                      \
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO1, &dummy1); \
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO2, &dummy2); \
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO3, &dummy3); \
  TEST_ASSERT_EQUAL(OK, nxscope.init())

//***************************************************************************
// Description: nxscope dummy proto reports hasThread() false before init.
//***************************************************************************

static void test_proto_nxscope_dummy_idle_no_thread()
{
  NXSCOPE_DUMMY_FIXTURE;

  TEST_ASSERT_FALSE(nxscope.hasThread());
}

//***************************************************************************
// Description: nxscope dummy proto runs through start -> hasThread -> stop.
//***************************************************************************

static void test_proto_nxscope_dummy_lifecycle()
{
  NXSCOPE_DUMMY_FIXTURE;

  TEST_ASSERT_EQUAL(OK, nxscope.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_TRUE(nxscope.hasThread());

  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_FALSE(nxscope.hasThread());
}

extern "C"
{
  int test_proto_nxscope_dummy()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_proto_nxscope_dummy_idle_no_thread);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_lifecycle);

    return UNITY_END();
  }
}
