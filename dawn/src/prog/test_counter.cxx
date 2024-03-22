// dawn/tests/prog/test_counter.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/prog/counter.hxx"
#include "test_common.hxx"

#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"

using namespace dawn;

static constexpr auto CTR_SRC = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 80);
static constexpr auto CTR_DST = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 81);

static uint32_t g_cfg_src[] = {CTR_SRC, 0};
static uint32_t g_cfg_dst[] = {CTR_DST, 0};

// Counter with min=0, max=3, step=1
static uint32_t g_bin_counter[] = {
  CProgCounter::objectId(0),
  2,
  CProgCounter::cfgIdIOBind(2),
  CTR_SRC,
  CTR_DST,
  CProgCounter::cfgIdParams(),
  0, // min
  3, // max
  1, // step
  0, // initial (not used, taken from min)
};

static void test_counter_increment()
{
  CDescObject srcDesc(g_cfg_src);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_dst);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_counter);
  CProgCounter prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(CTR_SRC, &src);
  prog.setObjectMapItem(CTR_DST, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  // first edge: 0→1
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  // second edge
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(2, out(0));

  // third → 3
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(3, out(0));

  // fourth → wrap to 0
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(0, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

static void test_counter_no_edge_on_high()
{
  CDescObject srcDesc(g_cfg_src);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_dst);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_counter);
  CProgCounter prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(CTR_SRC, &src);
  prog.setObjectMapItem(CTR_DST, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  // Sending 1 twice without falling edge doesn't increment
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0)); // only one increment

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

static void test_counter_reset()
{
  CDescObject srcDesc(g_cfg_src);
  CIOVirt src(srcDesc);
  CDescObject dstDesc(g_cfg_dst);
  CIOVirt dst(dstDesc);
  CDescObject progDesc(g_bin_counter);
  CProgCounter prog(progDesc);

  io_sdata_t<uint32_t, 1, 1> in;
  io_sdata_t<uint32_t, 1, 1> out;

  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, dst.init());
  TEST_ASSERT_EQUAL(OK, src.initialize(1, 1, true));
  TEST_ASSERT_EQUAL(OK, prog.configure());
  prog.setObjectMapItem(CTR_SRC, &src);
  prog.setObjectMapItem(CTR_DST, &dst);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());

  // count to 2
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));

  // reset
  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_RESET));

  // Verify count was reset by checking the next edge.
  // After reset, count=min, prevInput=0. Next 0→1 edge
  // increments to min+step = 1.
  in(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  in(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(in));
  TEST_ASSERT_EQUAL(OK, dst.getData(out, 1));
  TEST_ASSERT_EQUAL(1, out(0));

  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_counter()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_counter_increment);
    DAWN_RUN_TEST(test_counter_no_edge_on_high);
    DAWN_RUN_TEST(test_counter_reset);
    return UNITY_END();
  }
}
