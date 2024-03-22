// dawn/tests/prog/test_latest.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/latest.hxx"
#include "test_process_common.hxx"

static uint32_t g_bin_latest[] = {
  CProgLatest::objectId(0),
  1,
  CProgLatest::cfgIdIOBind(4),
  PROCESS_DUMMYIO1,
  PROCESS_VIRTIO1,
  PROCESS_DUMMYIO2,
  PROCESS_VIRTIO2,
};

// Configure + init two CIODummyNotify sources, two CIOVirt outputs, and
// the latest program; bind notifier and IO map; start prog + notifier.
// Caller drives setData on src1/src2 and stops everything.

#define LATEST_FIXTURE                            \
  CDescObject desc1(g_cfg_process_dummy1);        \
  CIODummyNotify src1(desc1);                     \
  CDescObject desc1b(g_cfg_process_dummy2);       \
  CIODummyNotify src2(desc1b);                    \
  CDescObject desc2(g_cfg_process_virt1);         \
  CIOVirt virt1(desc2);                           \
  CDescObject desc2b(g_cfg_process_virt2);        \
  CIOVirt virt2(desc2b);                          \
  CDescObject desc3(g_bin_latest);                \
  CProgLatest prog(desc3);                        \
  CIONotifier notifier;                           \
  TEST_ASSERT_EQUAL(OK, src1.configure());        \
  TEST_ASSERT_EQUAL(OK, src1.init());             \
  TEST_ASSERT_EQUAL(OK, src2.configure());        \
  TEST_ASSERT_EQUAL(OK, src2.init());             \
  TEST_ASSERT_EQUAL(OK, virt1.init());            \
  TEST_ASSERT_EQUAL(OK, virt2.init());            \
  TEST_ASSERT_EQUAL(OK, prog.configure());        \
  src1.bindNotifier(&notifier);                   \
  src2.bindNotifier(&notifier);                   \
  prog.setObjectMapItem(PROCESS_DUMMYIO1, &src1); \
  prog.setObjectMapItem(PROCESS_VIRTIO1, &virt1); \
  prog.setObjectMapItem(PROCESS_DUMMYIO2, &src2); \
  prog.setObjectMapItem(PROCESS_VIRTIO2, &virt2); \
  TEST_ASSERT_EQUAL(OK, prog.init());             \
  TEST_ASSERT_EQUAL(OK, prog.start());            \
  TEST_ASSERT_EQUAL(OK, notifier.start())

//***************************************************************************
// Description: with the initial src data (0..9) cached, virt outputs match
// the source values element-for-element.
//***************************************************************************

static void test_prog_latest_caches_initial_samples()
{
  LATEST_FIXTURE;
  io_sdata_t<uint32_t, 10, 1> data1;
  io_sdata_t<uint32_t, 10, 1> data2;

  process_set_dummy_data(src1, 0);
  process_set_dummy_data(src2, 0);

  TEST_ASSERT_EQUAL(OK, virt1.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, virt2.getData(data2, 1));
  TEST_ASSERT_EQUAL(0, data1(0));
  TEST_ASSERT_EQUAL(9, data1(9));
  TEST_ASSERT_EQUAL(0, data2(0));
  TEST_ASSERT_EQUAL(9, data2(9));

  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: each new sample replaces the cached data; the latest call
// to setData is the value that virt reads back.
//***************************************************************************

static void test_prog_latest_replaces_with_newest()
{
  LATEST_FIXTURE;
  io_sdata_t<uint32_t, 10, 1> data1;
  io_sdata_t<uint32_t, 10, 1> data2;

  process_set_dummy_data(src1, 1);
  process_set_dummy_data(src2, 1);
  process_set_dummy_data(src2, 2);

  TEST_ASSERT_EQUAL(OK, virt1.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, virt2.getData(data2, 1));
  TEST_ASSERT_EQUAL(1, data1(0));
  TEST_ASSERT_EQUAL(10, data1(9));
  TEST_ASSERT_EQUAL(2, data2(0));
  TEST_ASSERT_EQUAL(11, data2(9));

  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: a CMD_RESET trigger invalidates the cache; the next sample
// repopulates only the channels that produce new data.
//***************************************************************************

static void test_prog_latest_reset_clears_cache()
{
  LATEST_FIXTURE;
  io_sdata_t<uint32_t, 10, 1> data1;
  io_sdata_t<uint32_t, 10, 1> data2;

  process_set_dummy_data(src1, 1);
  process_set_dummy_data(src2, 2);

  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_RESET));

  process_set_dummy_data(src1, 2);

  TEST_ASSERT_EQUAL(OK, virt1.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, virt2.getData(data2, 1));
  TEST_ASSERT_EQUAL(2, data1(0));
  TEST_ASSERT_EQUAL(11, data1(9));
  TEST_ASSERT_EQUAL(2, data2(0));
  TEST_ASSERT_EQUAL(11, data2(9));

  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_latest()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_latest_caches_initial_samples);
    DAWN_RUN_TEST(test_prog_latest_replaces_with_newest);
    DAWN_RUN_TEST(test_prog_latest_reset_clears_cache);

    return UNITY_END();
  }
}
