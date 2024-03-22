// dawn/tests/io/test_gpi.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/gpi.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: test descriptors for GPI
//***************************************************************************

static uint32_t g_cfg_gpi0[] = {
  // Device: /dev/gpio0

  CIOGpi::objectId(false, 0),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

static uint32_t g_cfg_gpi1[] = {
  // Device: /dev/gpio1

  CIOGpi::objectId(false, 2),
  1,
  CIOCommon::cfgIdDevno(),
  1,
};

static uint32_t g_cfg_gpi2[] = {
  // Device: /dev/gpio2

  CIOGpi::objectId(true, 3),
  1,
  CIOCommon::cfgIdDevno(),
  2,
};

//***************************************************************************
// Description: a non-inverted GPI without timestamp reads as low (0).
//***************************************************************************

static void test_io_gpi_non_inverted_reads_low()
{
  CDescObject desc(g_cfg_gpi0);
  CIOGpi gpi(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, gpi.configure());
  TEST_ASSERT_EQUAL(OK, gpi.init());

  TEST_ASSERT_EQUAL(OK, gpi.getData(data, 1));
  TEST_ASSERT_EQUAL(false, data(0));
  TEST_ASSERT_EQUAL(0, data[0]);
}

//***************************************************************************
// Description: an inverted GPI reads as high (1).
//***************************************************************************

static void test_io_gpi_inverted_reads_high()
{
  CDescObject desc(g_cfg_gpi1);
  CIOGpi gpi(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, gpi.configure());
  TEST_ASSERT_EQUAL(OK, gpi.init());

  TEST_ASSERT_EQUAL(OK, gpi.getData(data, 1));
  TEST_ASSERT_EQUAL(true, data(0));
  TEST_ASSERT_EQUAL(0, data[0]);
}

//***************************************************************************
// Description: a GPI with timestamps enabled reports a non-zero timestamp
// alongside the value.
//***************************************************************************

static void test_io_gpi_with_timestamp()
{
  CDescObject desc(g_cfg_gpi2);
  CIOGpi gpi(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, gpi.configure());
  TEST_ASSERT_EQUAL(OK, gpi.init());

  TEST_ASSERT_EQUAL(OK, gpi.getData(data, 1));
  TEST_ASSERT_EQUAL(false, data(0));
  TEST_ASSERT(data[0] != 0);
}

//***************************************************************************
// Description: GPI does not support batched reads.
//***************************************************************************

static void test_io_gpi_batch_unsupported()
{
  CDescObject desc(g_cfg_gpi0);
  CIOGpi gpi(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, gpi.configure());
  TEST_ASSERT_EQUAL(OK, gpi.init());

  TEST_ASSERT_FALSE(gpi.isBatch());
  TEST_ASSERT_EQUAL(-ENOTSUP, gpi.getData(data, 2));
}

extern "C"
{
  int test_io_gpi()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_gpi_non_inverted_reads_low);
    DAWN_RUN_TEST(test_io_gpi_inverted_reads_high);
    DAWN_RUN_TEST(test_io_gpi_with_timestamp);
    DAWN_RUN_TEST(test_io_gpi_batch_unsupported);

    return UNITY_END();
  }
}
