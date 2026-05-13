// dawn/tests/io/test_rgbled.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/rgbled.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: test descriptors for RGB LED.
//***************************************************************************

static uint32_t g_cfg_rgbled0[] = {
  // Device: /dev/rgbled0

  CIORgbLed::objectId(false, 0),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

static uint32_t g_cfg_rgbled0_init[] = {
  // Device: /dev/rgbled0, initial color 0x112233.

  CIORgbLed::objectId(false, 1),
  2,
  CIOCommon::cfgIdDevno(),
  0,
  CIORgbLed::cfgIdInitVal(),
  0x00112233,
};

static uint32_t g_cfg_rgbled1_ts[] = {
  // Device: /dev/rgbled1, timestamp-enabled object.

  CIORgbLed::objectId(true, 2),
  1,
  CIOCommon::cfgIdDevno(),
  1,
};

//***************************************************************************
// Description: setData on RGB LED is reflected by getData.
//***************************************************************************

static void test_io_rgbled_set_get()
{
  CDescObject desc(g_cfg_rgbled0);
  CIORgbLed led(desc);
  io_sdata_t<uint32_t, 1> data;
  io_sdata_t<uint32_t, 1> rdata;
  uint32_t values[3] = {0x00000000, 0x00010203, 0x00ff8040};
  size_t i;

  TEST_ASSERT_EQUAL(OK, led.configure());
  TEST_ASSERT_EQUAL(OK, led.init());
  TEST_ASSERT_TRUE(led.isRead());
  TEST_ASSERT_TRUE(led.isWrite());
  TEST_ASSERT_FALSE(led.isBatch());
  TEST_ASSERT_EQUAL(sizeof(uint32_t), led.getDataSize());
  TEST_ASSERT_EQUAL(1u, led.getDataDim());

  for (i = 0; i < 3; i++)
    {
      data(0) = values[i];
      TEST_ASSERT_EQUAL(OK, led.setData(data));
      TEST_ASSERT_EQUAL(OK, led.getData(rdata, 1));
      TEST_ASSERT_EQUAL_UINT32(values[i], rdata(0));
    }
}

//***************************************************************************
// Description: init_val descriptor config is written during init().
//***************************************************************************

static void test_io_rgbled_initval()
{
  CDescObject desc(g_cfg_rgbled0_init);
  CIORgbLed led(desc);
  io_sdata_t<uint32_t, 1> rdata;

  TEST_ASSERT_EQUAL(OK, led.configure());
  TEST_ASSERT_EQUAL(OK, led.init());
  TEST_ASSERT_EQUAL(OK, led.getData(rdata, 1));
  TEST_ASSERT_EQUAL_UINT32(0x00112233, rdata(0));
}

//***************************************************************************
// Description: values are masked to the 24 RGB payload bits.
//***************************************************************************

static void test_io_rgbled_masks_to_24_bits()
{
  CDescObject desc(g_cfg_rgbled0);
  CIORgbLed led(desc);
  io_sdata_t<uint32_t, 1> data;
  io_sdata_t<uint32_t, 1> rdata;

  TEST_ASSERT_EQUAL(OK, led.configure());
  TEST_ASSERT_EQUAL(OK, led.init());

  data(0) = 0xffabcdef;
  TEST_ASSERT_EQUAL(OK, led.setData(data));
  TEST_ASSERT_EQUAL(OK, led.getData(rdata, 1));
  TEST_ASSERT_EQUAL_UINT32(0x00abcdef, rdata(0));
}

//***************************************************************************
// Description: timestamp-enabled RGB LED accepts the normal set/get path.
//***************************************************************************

static void test_io_rgbled_with_ts()
{
  CDescObject desc(g_cfg_rgbled1_ts);
  CIORgbLed led(desc);
  io_sdata_t<uint32_t, 1> data;
  io_sdata_t<uint32_t, 1> rdata;

  TEST_ASSERT_EQUAL(OK, led.configure());
  TEST_ASSERT_EQUAL(OK, led.init());

  data(0) = 0x0000ff00;
  TEST_ASSERT_EQUAL(OK, led.setData(data));
  TEST_ASSERT_EQUAL(OK, led.getData(rdata, 1));
  TEST_ASSERT_EQUAL_UINT32(0x0000ff00, rdata(0));
}

//***************************************************************************
// Description: RGB LED IO does not support batched reads.
//***************************************************************************

static void test_io_rgbled_batch_unsupported()
{
  CDescObject desc(g_cfg_rgbled0);
  CIORgbLed led(desc);
  io_sdata_t<uint32_t, 2> rdata_batch;

  TEST_ASSERT_EQUAL(OK, led.configure());
  TEST_ASSERT_EQUAL(OK, led.init());

  TEST_ASSERT_EQUAL(-ENOTSUP, led.getData(rdata_batch, 2));
}

extern "C"
{
  int test_io_rgbled()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_rgbled_set_get);
    DAWN_RUN_TEST(test_io_rgbled_initval);
    DAWN_RUN_TEST(test_io_rgbled_masks_to_24_bits);
    DAWN_RUN_TEST(test_io_rgbled_with_ts);
    DAWN_RUN_TEST(test_io_rgbled_batch_unsupported);

    return UNITY_END();
  }
}
