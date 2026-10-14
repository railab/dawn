// dawn/tests/prog/test_sampling.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dummy.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/sampling.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto SAMPLING_DUMMYIO1 = CIODummy::objectId(SObjectId::DTYPE_INT8, false, 0);
static constexpr auto SAMPLING_DUMMYIO2 = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto SAMPLING_DUMMYIO3 = CIODummy::objectId(SObjectId::DTYPE_FLOAT, false, 0);

static constexpr auto SAMPLING_VIRTIO1 = CIOVirt::objectId(SObjectId::DTYPE_INT8, false, 0);
static constexpr auto SAMPLING_VIRTIO2 = CIOVirt::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto SAMPLING_VIRTIO3 = CIOVirt::objectId(SObjectId::DTYPE_FLOAT, false, 0);
static constexpr auto SAMPLING_DUMMY_OUT = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 1);

static uint32_t g_cfg_dummy1[] = {
  SAMPLING_DUMMYIO1,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT8, true, 1),
  7,
};

static uint32_t g_cfg_dummy2[] = {
  SAMPLING_DUMMYIO2,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  11,
};

static uint32_t g_cfg_dummy3[] = {
  SAMPLING_DUMMYIO3,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_FLOAT, true, 1),
  SObjectCfg::fToCfg(3.5f),
};

static uint32_t g_cfg_virt1[] = {
  SAMPLING_VIRTIO1,
  0,
};

static uint32_t g_cfg_virt2[] = {
  SAMPLING_VIRTIO2,
  0,
};

static uint32_t g_cfg_virt3[] = {
  SAMPLING_VIRTIO3,
  0,
};

static uint32_t g_cfg_dummy_out[] = {
  SAMPLING_DUMMY_OUT,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
};

// sampling program

uint32_t g_bin_sampling_min[] = {
  // Object ID

  CProgSampling::objectId(0),
  2,

  // Allocated objects

  CProgSampling::cfgIdIOBind(6),
  SAMPLING_DUMMYIO1,
  SAMPLING_VIRTIO1,
  SAMPLING_DUMMYIO2,
  SAMPLING_VIRTIO2,
  SAMPLING_DUMMYIO3,
  SAMPLING_VIRTIO3,

  // Sampling interval

  CProgSampling::cfgIdIOInterval(),
  10000};

uint32_t g_bin_sampling_generic_target[] = {
  // Object ID

  CProgSampling::objectId(1),
  2,

  // Allocated objects

  CProgSampling::cfgIdIOBind(2),
  SAMPLING_DUMMYIO2,
  SAMPLING_DUMMY_OUT,

  // Sampling interval

  CProgSampling::cfgIdIOInterval(),
  10000};

// Configure + init the three source dummies, three virt IOs, and the
// sampling program; bind everything and finalize sampling init.  Caller
// drives start/stop and the post-stop assertions.

#define SAMPLING_FIXTURE                               \
  CDescObject desc1(g_cfg_dummy1);                     \
  CIODummy src1(desc1);                                \
  CDescObject desc2(g_cfg_dummy2);                     \
  CIODummy src2(desc2);                                \
  CDescObject desc3(g_cfg_dummy3);                     \
  CIODummy src3(desc3);                                \
  CDescObject descv1(g_cfg_virt1);                     \
  CIOVirt virt1(descv1);                               \
  CDescObject descv2(g_cfg_virt2);                     \
  CIOVirt virt2(descv2);                               \
  CDescObject descv3(g_cfg_virt3);                     \
  CIOVirt virt3(descv3);                               \
  CDescObject descs(g_bin_sampling_min);               \
  CProgSampling sampling(descs);                       \
  TEST_ASSERT_EQUAL(OK, src1.configure());             \
  TEST_ASSERT_EQUAL(OK, src2.configure());             \
  TEST_ASSERT_EQUAL(OK, src3.configure());             \
  TEST_ASSERT_EQUAL(OK, src1.init());                  \
  TEST_ASSERT_EQUAL(OK, src2.init());                  \
  TEST_ASSERT_EQUAL(OK, src3.init());                  \
  TEST_ASSERT_EQUAL(OK, virt1.init());                 \
  TEST_ASSERT_EQUAL(OK, virt2.init());                 \
  TEST_ASSERT_EQUAL(OK, virt3.init());                 \
  TEST_ASSERT_EQUAL(OK, sampling.configure());         \
  sampling.setObjectMapItem(SAMPLING_DUMMYIO1, &src1); \
  sampling.setObjectMapItem(SAMPLING_DUMMYIO2, &src2); \
  sampling.setObjectMapItem(SAMPLING_DUMMYIO3, &src3); \
  sampling.setObjectMapItem(SAMPLING_VIRTIO1, &virt1); \
  sampling.setObjectMapItem(SAMPLING_VIRTIO2, &virt2); \
  sampling.setObjectMapItem(SAMPLING_VIRTIO3, &virt3); \
  TEST_ASSERT_EQUAL(OK, sampling.init())

//***************************************************************************
// Description: sampling program runs through start -> hasThread -> stop.
//***************************************************************************

static void test_prog_sampling_lifecycle()
{
  SAMPLING_FIXTURE;

  TEST_ASSERT_FALSE(sampling.hasThread());
  TEST_ASSERT_EQUAL(OK, sampling.start());
  TEST_ASSERT_TRUE(sampling.hasThread());
  TEST_ASSERT_EQUAL(OK, sampling.stop());
  TEST_ASSERT_FALSE(sampling.hasThread());

  TEST_ASSERT_EQUAL(OK, sampling.deinit());
}

//***************************************************************************
// Description: after a brief run the sampling thread has populated each
// bound virt IO with data readable via getData().
//***************************************************************************

static void test_prog_sampling_writes_virt_ios()
{
  SAMPLING_FIXTURE;
  io_sdata_t<int8_t, 1, 1> data1;
  io_sdata_t<int32_t, 1, 1> data2;
  io_sdata_t<float, 1, 1> data3;

  TEST_ASSERT_EQUAL(OK, sampling.start());
  usleep(60000); // ~6 sampling intervals (interval = 10000 us)
  TEST_ASSERT_EQUAL(OK, sampling.stop());

  TEST_ASSERT_EQUAL(OK, virt1.getData(data1, 1));
  TEST_ASSERT_EQUAL(OK, virt2.getData(data2, 1));
  TEST_ASSERT_EQUAL(OK, virt3.getData(data3, 1));

  TEST_ASSERT_EQUAL(OK, sampling.deinit());
}

//***************************************************************************
// Description: sampling writes through the generic CIOCommon setData path.
//***************************************************************************

static void test_prog_sampling_writes_generic_output()
{
  CDescObject desc2(g_cfg_dummy2);
  CIODummy src2(desc2);
  CDescObject descout(g_cfg_dummy_out);
  CIODummy target(descout);
  CDescObject descs(g_bin_sampling_generic_target);
  CProgSampling sampling(descs);
  io_sdata_t<int32_t, 1, 1> data;

  TEST_ASSERT_EQUAL(OK, src2.configure());
  TEST_ASSERT_EQUAL(OK, target.configure());
  TEST_ASSERT_EQUAL(OK, src2.init());
  TEST_ASSERT_EQUAL(OK, target.init());
  TEST_ASSERT_EQUAL(OK, sampling.configure());
  sampling.setObjectMapItem(SAMPLING_DUMMYIO2, &src2);
  sampling.setObjectMapItem(SAMPLING_DUMMY_OUT, &target);
  TEST_ASSERT_EQUAL(OK, sampling.init());

  TEST_ASSERT_EQUAL(OK, sampling.start());
  usleep(60000);
  TEST_ASSERT_EQUAL(OK, sampling.stop());

  TEST_ASSERT_EQUAL(OK, target.getData(data, 1));
  TEST_ASSERT_EQUAL(11, *static_cast<int32_t *>(data.getDataPtr()));

  TEST_ASSERT_EQUAL(OK, sampling.deinit());
}

uint32_t g_bin_sampling_zero[] = {
  CProgSampling::objectId(2),
  2,
  CProgSampling::cfgIdIOBind(2),
  SAMPLING_DUMMYIO2,
  SAMPLING_DUMMY_OUT,
  CProgSampling::cfgIdIOInterval(),
  0,
};

// Dummy source whose read blocks for one system tick

class CIOBlockingDummy : public CIODummy
{
public:
  using CIODummy::CIODummy;

  int getDataImpl(IODataCmn &data, size_t len) override
  {
    usleep(CONFIG_USEC_PER_TICK);
    return CIODummy::getDataImpl(data, len);
  }
};

//***************************************************************************
// Description: a zero interval samples back to back: the blocking read
// alone paces the loop.
//***************************************************************************

static void test_prog_sampling_interval_zero_back_to_back()
{
  CDescObject desc2(g_cfg_dummy2);
  CIOBlockingDummy src2(desc2);
  CDescObject descout(g_cfg_dummy_out);
  CIODummy target(descout);
  CDescObject descs(g_bin_sampling_zero);
  CProgSampling sampling(descs);

  TEST_ASSERT_EQUAL(OK, src2.configure());
  TEST_ASSERT_EQUAL(OK, target.configure());
  TEST_ASSERT_EQUAL(OK, src2.init());
  TEST_ASSERT_EQUAL(OK, target.init());
  TEST_ASSERT_EQUAL(OK, sampling.configure());
  sampling.setObjectMapItem(SAMPLING_DUMMYIO2, &src2);
  sampling.setObjectMapItem(SAMPLING_DUMMY_OUT, &target);
  TEST_ASSERT_EQUAL(OK, sampling.init());
  target.resetStats();

  // ~20 reads in 20 ticks; any wait between reads would cut that down

  TEST_ASSERT_EQUAL(OK, sampling.start());
  usleep(20 * CONFIG_USEC_PER_TICK);
  TEST_ASSERT_EQUAL(OK, sampling.stop());

  TEST_ASSERT_TRUE(target.getStats().write_count >= 10);

  TEST_ASSERT_EQUAL(OK, sampling.deinit());
}

uint32_t g_bin_sampling_slow[] = {
  CProgSampling::objectId(3),
  2,
  CProgSampling::cfgIdIOBind(2),
  SAMPLING_DUMMYIO2,
  SAMPLING_DUMMY_OUT,
  CProgSampling::cfgIdIOInterval(),
  200000,
};

//***************************************************************************
// Description: the interval is runtime-writable; a shorter one written
// through setObjConfig applies after the current period.
//***************************************************************************

static void test_prog_sampling_runtime_interval_update()
{
  CDescObject desc2(g_cfg_dummy2);
  CIODummy src2(desc2);
  CDescObject descout(g_cfg_dummy_out);
  CIODummy target(descout);
  CDescObject descs(g_bin_sampling_slow);
  CProgSampling sampling(descs);
  uint32_t interval = 5000;

  TEST_ASSERT_EQUAL(OK, src2.configure());
  TEST_ASSERT_EQUAL(OK, target.configure());
  TEST_ASSERT_EQUAL(OK, src2.init());
  TEST_ASSERT_EQUAL(OK, target.init());
  TEST_ASSERT_EQUAL(OK, sampling.configure());
  sampling.setObjectMapItem(SAMPLING_DUMMYIO2, &src2);
  sampling.setObjectMapItem(SAMPLING_DUMMY_OUT, &target);
  TEST_ASSERT_EQUAL(OK, sampling.init());
  target.resetStats();

  TEST_ASSERT_EQUAL(OK, sampling.start());
  TEST_ASSERT_EQUAL(OK, sampling.setObjConfig(CProgSampling::cfgIdIOInterval(), &interval, 1));
  usleep(300000);
  TEST_ASSERT_EQUAL(OK, sampling.stop());

  // 200 ms alone gives 2 samples; the new 5 ms interval many more

  TEST_ASSERT_TRUE(target.getStats().write_count >= 10);

  TEST_ASSERT_EQUAL(OK, sampling.deinit());
}

// 20 ms interval, a multiple of the system tick, for the rate check.

uint32_t g_bin_sampling_rate[] = {
  CProgSampling::objectId(4),
  2,
  CProgSampling::cfgIdIOBind(2),
  SAMPLING_DUMMYIO2,
  SAMPLING_DUMMY_OUT,
  CProgSampling::cfgIdIOInterval(),
  20000,
};

//***************************************************************************
// Description: the period does not drift: work time and timer rounding do
// not add to the interval, so 1 s at 20 ms gives ~50 samples, not ~33.
//***************************************************************************

static void test_prog_sampling_keeps_rate()
{
  CDescObject desc2(g_cfg_dummy2);
  CIODummy src2(desc2);
  CDescObject descout(g_cfg_dummy_out);
  CIODummy target(descout);
  CDescObject descs(g_bin_sampling_rate);
  CProgSampling sampling(descs);
  size_t writes;

  TEST_ASSERT_EQUAL(OK, src2.configure());
  TEST_ASSERT_EQUAL(OK, target.configure());
  TEST_ASSERT_EQUAL(OK, src2.init());
  TEST_ASSERT_EQUAL(OK, target.init());
  TEST_ASSERT_EQUAL(OK, sampling.configure());
  sampling.setObjectMapItem(SAMPLING_DUMMYIO2, &src2);
  sampling.setObjectMapItem(SAMPLING_DUMMY_OUT, &target);
  TEST_ASSERT_EQUAL(OK, sampling.init());
  target.resetStats();

  TEST_ASSERT_EQUAL(OK, sampling.start());
  usleep(1000000);
  TEST_ASSERT_EQUAL(OK, sampling.stop());

  writes = target.getStats().write_count;
  TEST_ASSERT_TRUE_MESSAGE(writes >= 47 && writes <= 52, "sampling rate drifted");

  TEST_ASSERT_EQUAL(OK, sampling.deinit());
}

// Sampling thread above the default priority.

uint32_t g_bin_sampling_prio[] = {
  CProgSampling::objectId(5),
  3,
  CProgSampling::cfgIdIOBind(2),
  SAMPLING_DUMMYIO2,
  SAMPLING_DUMMY_OUT,
  CProgSampling::cfgIdIOInterval(),
  10000,
  CProgSampling::cfgIdPriority(),
  110,
};

// Priority item with two words.

uint32_t g_bin_sampling_prio_bad[] = {
  CProgSampling::objectId(6),
  1,
  CProgSampling::cfgId(false, 2, CProgSampling::PROG_SAMPLING_CFG_PRIORITY),
  110,
  0,
};

//***************************************************************************
// Description: a configured thread priority is accepted and sampling runs;
// a malformed priority item is rejected.
//***************************************************************************

static void test_prog_sampling_priority()
{
  CDescObject desc2(g_cfg_dummy2);
  CIODummy src2(desc2);
  CDescObject descout(g_cfg_dummy_out);
  CIODummy target(descout);
  CDescObject descs(g_bin_sampling_prio);
  CProgSampling sampling(descs);
  CDescObject descbad(g_bin_sampling_prio_bad);
  CProgSampling bad(descbad);

  TEST_ASSERT_EQUAL(OK, src2.configure());
  TEST_ASSERT_EQUAL(OK, target.configure());
  TEST_ASSERT_EQUAL(OK, src2.init());
  TEST_ASSERT_EQUAL(OK, target.init());
  TEST_ASSERT_EQUAL(OK, sampling.configure());
  sampling.setObjectMapItem(SAMPLING_DUMMYIO2, &src2);
  sampling.setObjectMapItem(SAMPLING_DUMMY_OUT, &target);
  TEST_ASSERT_EQUAL(OK, sampling.init());
  target.resetStats();

  TEST_ASSERT_EQUAL(-EINVAL, bad.configure());

  TEST_ASSERT_EQUAL(OK, sampling.start());
  usleep(60000);
  TEST_ASSERT_EQUAL(OK, sampling.stop());

  TEST_ASSERT_TRUE(target.getStats().write_count > 0);

  TEST_ASSERT_EQUAL(OK, sampling.deinit());
}

// Auto-start off: sampling waits for trigger1.

uint32_t g_bin_sampling_manual[] = {
  CProgSampling::objectId(7),
  3,
  CProgSampling::cfgIdIOBind(2),
  SAMPLING_DUMMYIO2,
  SAMPLING_DUMMY_OUT,
  CProgSampling::cfgIdIOInterval(),
  5000,
  CProgSampling::cfgIdFlags(),
  0,
};

// Unknown flag bit.

uint32_t g_bin_sampling_flags_bad[] = {
  CProgSampling::objectId(8),
  1,
  CProgSampling::cfgIdFlags(),
  0x2,
};

//***************************************************************************
// Description: with auto-start off nothing is sampled until trigger1;
// trigger2 stops sampling while the program keeps running.
//***************************************************************************

static void test_prog_sampling_trigger_start_stop()
{
  CDescObject desc2(g_cfg_dummy2);
  CIODummy src2(desc2);
  CDescObject descout(g_cfg_dummy_out);
  CIODummy target(descout);
  CDescObject descs(g_bin_sampling_manual);
  CProgSampling sampling(descs);
  size_t writes;

  TEST_ASSERT_EQUAL(OK, src2.configure());
  TEST_ASSERT_EQUAL(OK, target.configure());
  TEST_ASSERT_EQUAL(OK, src2.init());
  TEST_ASSERT_EQUAL(OK, target.init());
  TEST_ASSERT_EQUAL(OK, sampling.configure());
  sampling.setObjectMapItem(SAMPLING_DUMMYIO2, &src2);
  sampling.setObjectMapItem(SAMPLING_DUMMY_OUT, &target);
  TEST_ASSERT_EQUAL(OK, sampling.init());
  target.resetStats();

  TEST_ASSERT_EQUAL(OK, sampling.start());
  usleep(50000);
  TEST_ASSERT_EQUAL(0, target.getStats().write_count);

  TEST_ASSERT_EQUAL(OK, sampling.trigger(CObject::CMD_TRIGGER1));
  usleep(50000);
  TEST_ASSERT_TRUE(target.getStats().write_count > 0);

  TEST_ASSERT_EQUAL(OK, sampling.trigger(CObject::CMD_TRIGGER2));
  usleep(20000);
  writes = target.getStats().write_count;
  usleep(50000);
  TEST_ASSERT_EQUAL(writes, target.getStats().write_count);
  TEST_ASSERT_TRUE(sampling.hasThread());

  TEST_ASSERT_EQUAL(-ENOTSUP, sampling.trigger(CObject::CMD_TRIGGER3));

  // Stop while waiting for a trigger must not hang

  TEST_ASSERT_EQUAL(OK, sampling.stop());
  TEST_ASSERT_FALSE(sampling.hasThread());

  TEST_ASSERT_EQUAL(OK, sampling.deinit());
}

//***************************************************************************
// Description: a restart applies the auto-start flag again; an unknown
// flag bit is rejected.
//***************************************************************************

static void test_prog_sampling_flags()
{
  CDescObject desc2(g_cfg_dummy2);
  CIODummy src2(desc2);
  CDescObject descout(g_cfg_dummy_out);
  CIODummy target(descout);
  CDescObject descs(g_bin_sampling_manual);
  CProgSampling sampling(descs);
  CDescObject descbad(g_bin_sampling_flags_bad);
  CProgSampling bad(descbad);

  TEST_ASSERT_EQUAL(OK, src2.configure());
  TEST_ASSERT_EQUAL(OK, target.configure());
  TEST_ASSERT_EQUAL(OK, src2.init());
  TEST_ASSERT_EQUAL(OK, target.init());
  TEST_ASSERT_EQUAL(OK, sampling.configure());
  sampling.setObjectMapItem(SAMPLING_DUMMYIO2, &src2);
  sampling.setObjectMapItem(SAMPLING_DUMMY_OUT, &target);
  TEST_ASSERT_EQUAL(OK, sampling.init());
  target.resetStats();

  TEST_ASSERT_EQUAL(-EINVAL, bad.configure());

  TEST_ASSERT_EQUAL(OK, sampling.start());
  TEST_ASSERT_EQUAL(OK, sampling.trigger(CObject::CMD_TRIGGER1));
  TEST_ASSERT_EQUAL(OK, sampling.stop());

  target.resetStats();
  TEST_ASSERT_EQUAL(OK, sampling.start());
  usleep(50000);
  TEST_ASSERT_EQUAL(0, target.getStats().write_count);
  TEST_ASSERT_EQUAL(OK, sampling.stop());

  TEST_ASSERT_EQUAL(OK, sampling.deinit());
}

extern "C"
{
  int test_prog_sampling()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_sampling_lifecycle);
    DAWN_RUN_TEST(test_prog_sampling_writes_virt_ios);
    DAWN_RUN_TEST(test_prog_sampling_writes_generic_output);
    DAWN_RUN_TEST(test_prog_sampling_interval_zero_back_to_back);
    DAWN_RUN_TEST(test_prog_sampling_runtime_interval_update);
    DAWN_RUN_TEST(test_prog_sampling_keeps_rate);
    DAWN_RUN_TEST(test_prog_sampling_priority);
    DAWN_RUN_TEST(test_prog_sampling_trigger_start_stop);
    DAWN_RUN_TEST(test_prog_sampling_flags);

    return UNITY_END();
  }
}
