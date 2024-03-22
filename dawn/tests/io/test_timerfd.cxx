// dawn/tests/io/test_timerfd.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/timerfd.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: initialize timerfd IO common logic
//***************************************************************************

static void test_io_timerfd_init()
{
  CIOTimerfd tim;

  // Not initialized

  TEST_ASSERT_EQUAL(-1, tim.timfd_fd());

  // No interval

  TEST_ASSERT_EQUAL(OK, tim.timfd_init());
  TEST_ASSERT_EQUAL(-1, tim.timfd_fd());
  TEST_ASSERT_EQUAL(OK, tim.timfd_start());

  // Valid interval

  tim.timfd_interval(10000);
  TEST_ASSERT_EQUAL(OK, tim.timfd_init());
  TEST_ASSERT_NOT_EQUAL(-1, tim.timfd_fd());
  TEST_ASSERT_EQUAL(OK, tim.timfd_start());

  // Stop timer

  TEST_ASSERT_EQUAL(OK, tim.timfd_stop());
  TEST_ASSERT_NOT_EQUAL(-1, tim.timfd_fd());
  TEST_ASSERT_EQUAL(OK, tim.timfd_start());
  TEST_ASSERT_EQUAL(OK, tim.timfd_stop());
}

extern "C"
{
  int test_io_timerfd()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_timerfd_init);

    return UNITY_END();
  }
}
