// dawn/tests/io/test_notifier.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <pthread.h>
#include <sched.h>

#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "test_common.hxx"
#include "test_iomockcommon.hxx"

using namespace dawn;

static uint32_t g_cfg_valid_io1[] = {CIOMockCommon::objectId(0), 0};

static uint32_t g_cfg_valid_io2[] = {
  CIODummyNotify::objectId(SObjectId::DTYPE_INT8, false, 2),
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT8, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_valid_io3[] = {
  CIODummyNotify::objectId(SObjectId::DTYPE_INT16, false, 3),
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT16, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_valid_io4[] = {
  CIODummyNotify::objectId(SObjectId::DTYPE_UINT32, false, 4),
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

// Callback counters

static int g_callback1_cntr = 0;
static int g_callback2_cntr = 0;
static int g_callback3_cntr = 0;
static int g_callback4_cntr = 0;
static int g_callback5_cntr = 0;
static int g_callback6_cntr = 0;
static int g_callback_sched_ret = -1;
static int g_callback_sched_policy = CThreadedObject::THREAD_SCHEDULER_DEFAULT;
static int g_callback_sched_priority = CThreadedObject::THREAD_PRIORITY_DEFAULT;

static int notifier_callback1(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  g_callback1_cntr++;
  return OK;
}

static int notifier_callback2(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  g_callback2_cntr++;
  return OK;
}

static int notifier_callback3(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  g_callback3_cntr++;
  return OK;
}

static int notifier_callback4(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  g_callback4_cntr++;
  return OK;
}

static int notifier_callback5(void *priv, io_ddata_t *data)
{
  DAWNASSERT(data != nullptr, "nullptr pointer");
  g_callback5_cntr++;
  return OK;
}

static int notifier_callback6(void *priv, io_ddata_t *data)
{
  struct sched_param param;
  int policy;

  DAWNASSERT(data != nullptr, "nullptr pointer");

  policy = CThreadedObject::THREAD_SCHEDULER_DEFAULT;
  param.sched_priority = CThreadedObject::THREAD_PRIORITY_DEFAULT;
  g_callback_sched_ret = pthread_getschedparam(pthread_self(), &policy, &param);
  g_callback_sched_policy = policy;
  g_callback_sched_priority = param.sched_priority;
  g_callback6_cntr++;
  return OK;
}

//***************************************************************************
// Description: regNotifier rejects an IO that doesn't support notification
// (CIOMockCommon) with -EINVAL.
//***************************************************************************

static void test_io_notifier_reg_rejects_unsupported_io()
{
  CDescObject desc1(g_cfg_valid_io1);
  CIOMockCommon mock1(desc1);
  CIONotifier notifier;
  IIONotifier::SIONotifier n;

  TEST_ASSERT_EQUAL(OK, mock1.init());

  n.io = &mock1;
  n.priv = nullptr;
  n.cb = notifier_callback1;
  n.prio = 0;
  TEST_ASSERT_EQUAL(-EINVAL, notifier.regNotifier(n));
}

//***************************************************************************
// Description: regNotifier accepts the same notifier-capable IO with
// multiple callbacks; both fire on the next notify.
//***************************************************************************

static void test_io_notifier_reg_multiple_callbacks_per_io()
{
  CDescObject desc4(g_cfg_valid_io4);
  CIODummyNotify mock4(desc4);
  CIONotifier notifier;
  IIONotifier::SIONotifier n;

  g_callback4_cntr = 0;
  g_callback5_cntr = 0;

  TEST_ASSERT_EQUAL(OK, mock4.configure());
  TEST_ASSERT_EQUAL(OK, mock4.init());

  n.io = &mock4;
  n.priv = nullptr;
  n.prio = 0;

  n.cb = notifier_callback4;
  TEST_ASSERT_EQUAL(OK, notifier.regNotifier(n));

  n.cb = notifier_callback5;
  TEST_ASSERT_EQUAL(OK, notifier.regNotifier(n));

  notifier.start();
  TEST_ASSERT_EQUAL(OK, mock4.start());
  usleep(6000);
  TEST_ASSERT_EQUAL(OK, mock4.stop());
  notifier.stop();

  TEST_ASSERT_EQUAL(1, g_callback4_cntr);
  TEST_ASSERT_EQUAL(1, g_callback5_cntr);
}

//***************************************************************************
// Description: notifying one IO fires only that IO's callback; other
// registered IOs' callbacks remain at zero.
//***************************************************************************

static void test_io_notifier_per_io_isolation()
{
  CDescObject desc2(g_cfg_valid_io2);
  CDescObject desc3(g_cfg_valid_io3);
  CIODummyNotify mock2(desc2);
  CIODummyNotify mock3(desc3);
  CIONotifier notifier;
  IIONotifier::SIONotifier n;

  g_callback2_cntr = 0;
  g_callback3_cntr = 0;

  TEST_ASSERT_EQUAL(OK, mock2.configure());
  TEST_ASSERT_EQUAL(OK, mock2.init());
  TEST_ASSERT_EQUAL(OK, mock3.configure());
  TEST_ASSERT_EQUAL(OK, mock3.init());

  n.io = &mock2;
  n.priv = nullptr;
  n.cb = notifier_callback2;
  n.prio = 0;
  TEST_ASSERT_EQUAL(OK, notifier.regNotifier(n));
  n.io = &mock3;
  n.cb = notifier_callback3;
  TEST_ASSERT_EQUAL(OK, notifier.regNotifier(n));

  notifier.start();

  // Notify mock2 only.

  TEST_ASSERT_EQUAL(OK, mock2.start());
  usleep(6000);
  TEST_ASSERT_EQUAL(OK, mock2.stop());

  TEST_ASSERT_EQUAL(1, g_callback2_cntr);
  TEST_ASSERT_EQUAL(0, g_callback3_cntr);

  notifier.stop();
}

//***************************************************************************
// Description: IO common interaction with notifier
//***************************************************************************

static void test_io_notifier_bind_set()
{
  CDescObject desc1(g_cfg_valid_io1);
  CIOMockCommon mock1(desc1);
  CDescObject desc2(g_cfg_valid_io2);
  CIODummyNotify mock2(desc2);
  CIONotifier notifier;
  int ret;

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, mock1.init());
  TEST_ASSERT_EQUAL(OK, mock2.configure());
  TEST_ASSERT_EQUAL(OK, mock2.init());

  // Notifier not binded

  ret = mock1.setNotifier(notifier_callback1, 1, nullptr);
  TEST_ASSERT_EQUAL(-EPERM, ret);
  ret = mock2.setNotifier(notifier_callback1, 1, nullptr);
  TEST_ASSERT_EQUAL(-EPERM, ret);

  // Bind notifer now

  mock1.bindNotifier(&notifier);
  mock2.bindNotifier(&notifier);

  // Mock1 doesnt support notifier

  ret = mock1.setNotifier(notifier_callback1, 1, nullptr);
  TEST_ASSERT_EQUAL(-EINVAL, ret);

  // Set few callbacks now

  ret = mock2.setNotifier(notifier_callback1, 1, nullptr);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = mock2.setNotifier(notifier_callback2, 1, nullptr);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = mock2.setNotifier(notifier_callback3, 1, nullptr);
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: notifier rejects new registrations after it has started.
//***************************************************************************

static void test_io_notifier_reject_new_registration_after_start()
{
  CDescObject desc2(g_cfg_valid_io2);
  CDescObject desc3(g_cfg_valid_io3);
  CIODummyNotify mock2(desc2);
  CIODummyNotify mock3(desc3);
  CIONotifier notifier;
  IIONotifier::SIONotifier n;
  int ret;

  TEST_ASSERT_EQUAL(OK, mock2.configure());
  TEST_ASSERT_EQUAL(OK, mock2.init());
  TEST_ASSERT_EQUAL(OK, mock3.configure());
  TEST_ASSERT_EQUAL(OK, mock3.init());

  n.io = &mock2;
  n.priv = nullptr;
  n.cb = notifier_callback2;
  n.prio = 0;
  ret = notifier.regNotifier(n);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = notifier.start();
  TEST_ASSERT_EQUAL(OK, ret);

  n.io = &mock3;
  n.cb = notifier_callback3;
  ret = notifier.regNotifier(n);
  TEST_ASSERT_EQUAL(-EPERM, ret);

  TEST_ASSERT_EQUAL(OK, notifier.stop());
}

//***************************************************************************
// Description: notifier thread applies configured worker priority.
//***************************************************************************

static void test_io_notifier_applies_configured_thread_priority()
{
  CDescObject desc4(g_cfg_valid_io4);
  CIODummyNotify mock4(desc4);
  CIONotifier notifier;
  IIONotifier::SIONotifier n;
  struct sched_param param;
  int desiredPriority;
  int maxPriority;
  int minPriority;
  int policy;
  int ret;

  g_callback6_cntr = 0;
  g_callback_sched_ret = -1;
  g_callback_sched_policy = CThreadedObject::THREAD_SCHEDULER_DEFAULT;
  g_callback_sched_priority = CThreadedObject::THREAD_PRIORITY_DEFAULT;

  TEST_ASSERT_EQUAL(OK, mock4.configure());
  TEST_ASSERT_EQUAL(OK, mock4.init());

  ret = pthread_getschedparam(pthread_self(), &policy, &param);
  TEST_ASSERT_EQUAL(0, ret);

  minPriority = sched_get_priority_min(policy);
  maxPriority = sched_get_priority_max(policy);
  TEST_ASSERT_TRUE(minPriority >= 0);
  TEST_ASSERT_TRUE(maxPriority >= 0);

  desiredPriority = param.sched_priority;
  if (desiredPriority < maxPriority)
    {
      desiredPriority++;
    }
  else if (desiredPriority > minPriority)
    {
      desiredPriority--;
    }

  notifier.setThreadPriority(desiredPriority);

  n.io = &mock4;
  n.priv = nullptr;
  n.cb = notifier_callback6;
  n.prio = 0;
  TEST_ASSERT_EQUAL(OK, notifier.regNotifier(n));

  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, mock4.start());
  usleep(6000);
  TEST_ASSERT_EQUAL(OK, mock4.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());

  TEST_ASSERT_EQUAL(1, g_callback6_cntr);
  TEST_ASSERT_EQUAL(0, g_callback_sched_ret);
  TEST_ASSERT_EQUAL(policy, g_callback_sched_policy);
  TEST_ASSERT_EQUAL(desiredPriority, g_callback_sched_priority);
}

extern "C"
{
  int test_io_notifier()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_notifier_reg_rejects_unsupported_io);
    DAWN_RUN_TEST(test_io_notifier_reg_multiple_callbacks_per_io);
    DAWN_RUN_TEST(test_io_notifier_per_io_isolation);
    DAWN_RUN_TEST(test_io_notifier_bind_set);
    DAWN_RUN_TEST(test_io_notifier_reject_new_registration_after_start);
    DAWN_RUN_TEST(test_io_notifier_applies_configured_thread_priority);

    return UNITY_END();
  }
}
