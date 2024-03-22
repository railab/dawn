// dawn/tests/io/test_leds.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/leds.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: test descriptors for Leds
//***************************************************************************

static uint32_t g_cfg_led0[] = {
  // Device: /dev/leds0

  CIOLeds::objectId(false, 0),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

static uint32_t g_cfg_led1[] = {
  // Device: /dev/leds1

  CIOLeds::objectId(false, 2),
  1,
  CIOCommon::cfgIdDevno(),
  1,
};

static uint32_t g_cfg_led2[] = {
  // Device: /dev/leds2

  CIOLeds::objectId(true, 3),
  1,
  CIOCommon::cfgIdDevno(),
  2,
};

//***************************************************************************
// Description: Configure + init the supplied LED IO and verify isRead() is
// true. Drives a sequence of setData/getData round-trips and checks the read
// matches each set.
//***************************************************************************

static void leds_set_get_sequence(CIOLeds &led)
{
  io_sdata_t<uint32_t, 1> data;
  io_sdata_t<uint32_t, 1> rdata;
  uint32_t values[3] = {1, 2, 3};
  size_t i;

  TEST_ASSERT_EQUAL(OK, led.configure());
  TEST_ASSERT_EQUAL(OK, led.init());
  TEST_ASSERT_TRUE(led.isRead());

  for (i = 0; i < 3; i++)
    {
      data(0) = values[i];
      TEST_ASSERT_EQUAL(OK, led.setData(data));
      TEST_ASSERT_EQUAL(OK, led.getData(rdata, 1));
      TEST_ASSERT_EQUAL_UINT32(values[i], rdata(0));
    }
}

//***************************************************************************
// Description: setData on a non-inverted LED is reflected by getData.
//***************************************************************************

static void test_io_leds_devno0_set_get()
{
  CDescObject desc(g_cfg_led0);
  CIOLeds led(desc);

  leds_set_get_sequence(led);
}

//***************************************************************************
// Description: setData on a different non-inverted LED instance is
// reflected by getData.
//***************************************************************************

static void test_io_leds_devno1_set_get()
{
  CDescObject desc(g_cfg_led1);
  CIOLeds led(desc);

  leds_set_get_sequence(led);
}

//***************************************************************************
// Description: setData on a timestamp-enabled LED is reflected by getData.
//***************************************************************************

static void test_io_leds_devno2_set_get_with_ts()
{
  CDescObject desc(g_cfg_led2);
  CIOLeds led(desc);

  leds_set_get_sequence(led);
}

//***************************************************************************
// Description: LED IO does not support batched reads.
//***************************************************************************

static void test_io_leds_batch_unsupported()
{
  CDescObject desc(g_cfg_led0);
  CIOLeds led(desc);
  io_sdata_t<uint32_t, 2> rdata_batch;

  TEST_ASSERT_EQUAL(OK, led.configure());
  TEST_ASSERT_EQUAL(OK, led.init());

  TEST_ASSERT_EQUAL(-ENOTSUP, led.getData(rdata_batch, 2));
}

extern "C"
{
  int test_io_leds()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_leds_devno0_set_get);
    DAWN_RUN_TEST(test_io_leds_devno1_set_get);
    DAWN_RUN_TEST(test_io_leds_devno2_set_get_with_ts);
    DAWN_RUN_TEST(test_io_leds_batch_unsupported);

    return UNITY_END();
  }
}
