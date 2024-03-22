// dawn/tests/prog/test_redirect.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dummy.hxx"
#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/prog/redirect.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto REDIRECT_SRC1 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto REDIRECT_SRC2 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto REDIRECT_DST1 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 101);
static constexpr auto REDIRECT_DST2 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 102);

static uint32_t g_bin_redirect_notify[] = {
  CProgRedirect::objectId(0),
  1,
  CProgRedirect::cfgIdIOBind(4),
  REDIRECT_SRC1,
  REDIRECT_DST1,
  REDIRECT_SRC2,
  REDIRECT_DST2,
};

static uint32_t g_cfg_redirect_src1[] = {
  REDIRECT_SRC1,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  1000,
};

static uint32_t g_cfg_redirect_src2[] = {
  REDIRECT_SRC2,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  1000,
};

static uint32_t g_cfg_redirect_dst1[] = {REDIRECT_DST1, 0};
static uint32_t g_cfg_redirect_dst2[] = {REDIRECT_DST2, 0};

#define REDIRECT_FIXTURE                       \
  CDescObject desc1(g_cfg_redirect_src1);      \
  CIODummyNotify src1(desc1);                  \
  CDescObject desc2(g_cfg_redirect_src2);      \
  CIODummyNotify src2(desc2);                  \
  CDescObject descd1(g_cfg_redirect_dst1);     \
  CIODummy out1(descd1);                       \
  CDescObject descd2(g_cfg_redirect_dst2);     \
  CIODummy out2(descd2);                       \
  CDescObject descp(g_bin_redirect_notify);    \
  CProgRedirect prog(descp);                   \
  CIONotifier notifier;                        \
  TEST_ASSERT_EQUAL(OK, src1.configure());     \
  TEST_ASSERT_EQUAL(OK, src2.configure());     \
  TEST_ASSERT_EQUAL(OK, out1.configure());     \
  TEST_ASSERT_EQUAL(OK, out2.configure());     \
  TEST_ASSERT_EQUAL(OK, src1.init());          \
  TEST_ASSERT_EQUAL(OK, src2.init());          \
  TEST_ASSERT_EQUAL(OK, out1.init());          \
  TEST_ASSERT_EQUAL(OK, out2.init());          \
  TEST_ASSERT_EQUAL(OK, prog.configure());     \
  src1.bindNotifier(&notifier);                \
  src2.bindNotifier(&notifier);                \
  prog.setObjectMapItem(REDIRECT_SRC1, &src1); \
  prog.setObjectMapItem(REDIRECT_DST1, &out1); \
  prog.setObjectMapItem(REDIRECT_SRC2, &src2); \
  prog.setObjectMapItem(REDIRECT_DST2, &out2); \
  TEST_ASSERT_EQUAL(OK, prog.init())

//***************************************************************************
// Description: redirect program is callback-driven and start() does not
// spin up a thread (hasThread() stays false).
//***************************************************************************

static void test_prog_redirect_lifecycle_no_thread()
{
  REDIRECT_FIXTURE;

  TEST_ASSERT_FALSE(prog.hasThread());
  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_FALSE(prog.hasThread());
  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: a notification with value 0 forwards 0 from each src to its
// bound dst.
//***************************************************************************

static void test_prog_redirect_forward_zero()
{
  REDIRECT_FIXTURE;
  io_sdata_t<int32_t, 1, 1> src_data;
  io_sdata_t<int32_t, 1, 1> data1;
  io_sdata_t<int32_t, 1, 1> data2;

  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, src1.start());
  TEST_ASSERT_EQUAL(OK, src2.start());

  src_data(0) = 0;
  TEST_ASSERT_EQUAL(OK, src1.setData(src_data));
  TEST_ASSERT_EQUAL(OK, src2.setData(src_data));
  usleep(2000);

  TEST_ASSERT_EQUAL(OK, out1.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, out2.getData(data2, 1));
  TEST_ASSERT_EQUAL(0, data1(0));
  TEST_ASSERT_EQUAL(0, data2(0));

  TEST_ASSERT_EQUAL(OK, src1.stop());
  TEST_ASSERT_EQUAL(OK, src2.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: setting a non-zero value on each src is forwarded to the
// matching dst IO.
//***************************************************************************

static void test_prog_redirect_forward_nonzero()
{
  REDIRECT_FIXTURE;
  io_sdata_t<int32_t, 1, 1> src_data;
  io_sdata_t<int32_t, 1, 1> data1;
  io_sdata_t<int32_t, 1, 1> data2;

  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, src1.start());
  TEST_ASSERT_EQUAL(OK, src2.start());

  src_data(0) = 1;
  TEST_ASSERT_EQUAL(OK, src1.setData(src_data));
  TEST_ASSERT_EQUAL(OK, src2.setData(src_data));
  usleep(2000);

  TEST_ASSERT_EQUAL(OK, out1.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, out2.getData(data2, 1));
  TEST_ASSERT_EQUAL(1, data1(0));
  TEST_ASSERT_EQUAL(1, data2(0));

  TEST_ASSERT_EQUAL(OK, src1.stop());
  TEST_ASSERT_EQUAL(OK, src2.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_redirect()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_redirect_lifecycle_no_thread);
    DAWN_RUN_TEST(test_prog_redirect_forward_zero);
    DAWN_RUN_TEST(test_prog_redirect_forward_nonzero);

    return UNITY_END();
  }
}
