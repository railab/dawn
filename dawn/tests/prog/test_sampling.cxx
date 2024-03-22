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

extern "C"
{
  int test_prog_sampling()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_sampling_lifecycle);
    DAWN_RUN_TEST(test_prog_sampling_writes_virt_ios);
    DAWN_RUN_TEST(test_prog_sampling_writes_generic_output);

    return UNITY_END();
  }
}
