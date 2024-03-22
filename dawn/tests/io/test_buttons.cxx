// dawn/tests/io/test_buttons.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/buttons.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_btn0[] = {
  // Device: /dev/buttons0

  CIOButtons::objectId(false, 0),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

static uint32_t g_cfg_btn1[] = {
  // Device: /dev/buttons1

  CIOButtons::objectId(false, 2),
  1,
  CIOCommon::cfgIdDevno(),
  1,
};

static uint32_t g_cfg_btn2[] = {
  // Device: /dev/buttons2

  CIOButtons::objectId(true, 3),
  1,
  CIOCommon::cfgIdDevno(),
  2,
};

//***************************************************************************
// Description: Verify that consecutive getData() reads on the supplied
// button IO toggle the bit corresponding to its devno (mock contract).
//***************************************************************************

static void buttons_read_toggle(CIOButtons &btn, uint8_t devno, bool expect_ts)
{
  io_sdata_t<uint32_t, 1> data;
  uint32_t init;
  int i;

  TEST_ASSERT_EQUAL(OK, btn.getData(data, 1));
  init = data(0);
  if (expect_ts)
    {
      TEST_ASSERT(data[0] != 0);
    }
  else
    {
      TEST_ASSERT_EQUAL(0, data[0]);
    }

  for (i = 0; i < 3; i++)
    {
      TEST_ASSERT_EQUAL(OK, btn.getData(data, 1));
      TEST_ASSERT_EQUAL((1u << devno) ^ init, data(0));
      init = data(0);
      if (expect_ts)
        {
          TEST_ASSERT(data[0] != 0);
        }
      else
        {
          TEST_ASSERT_EQUAL(0, data[0]);
        }
    }
}

//***************************************************************************
// Description: buttons IO at devno 0 (no timestamp) toggles bit 0 between
// successive reads.
//***************************************************************************

static void test_io_buttons_devno0_toggle()
{
  CDescObject desc(g_cfg_btn0);
  CIOButtons btn(desc);

  TEST_ASSERT_EQUAL(OK, btn.configure());
  TEST_ASSERT_EQUAL(OK, btn.init());

  buttons_read_toggle(btn, 0, false);
}

//***************************************************************************
// Description: buttons IO at devno 1 (no timestamp) toggles bit 1 between
// successive reads.
//***************************************************************************

static void test_io_buttons_devno1_toggle()
{
  CDescObject desc(g_cfg_btn1);
  CIOButtons btn(desc);

  TEST_ASSERT_EQUAL(OK, btn.configure());
  TEST_ASSERT_EQUAL(OK, btn.init());

  buttons_read_toggle(btn, 1, false);
}

//***************************************************************************
// Description: buttons IO at devno 2 with timestamps enabled toggles bit 2
// and reports a non-zero timestamp on every read.
//***************************************************************************

static void test_io_buttons_devno2_toggle_with_ts()
{
  CDescObject desc(g_cfg_btn2);
  CIOButtons btn(desc);

  TEST_ASSERT_EQUAL(OK, btn.configure());
  TEST_ASSERT_EQUAL(OK, btn.init());

  buttons_read_toggle(btn, 2, true);
}

//***************************************************************************
// Description: buttons IO does not support batched reads; isBatch() is
// false and a multi-element getData() returns -ENOTSUP.
//***************************************************************************

static void test_io_buttons_batch_unsupported()
{
  CDescObject desc(g_cfg_btn0);
  CIOButtons btn(desc);
  io_sdata_t<uint32_t, 1> data;

  TEST_ASSERT_EQUAL(OK, btn.configure());
  TEST_ASSERT_EQUAL(OK, btn.init());

  TEST_ASSERT_FALSE(btn.isBatch());
  TEST_ASSERT_EQUAL(-ENOTSUP, btn.getData(data, 2));
}

extern "C"
{
  int test_io_buttons()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_buttons_devno0_toggle);
    DAWN_RUN_TEST(test_io_buttons_devno1_toggle);
    DAWN_RUN_TEST(test_io_buttons_devno2_toggle_with_ts);
    DAWN_RUN_TEST(test_io_buttons_batch_unsupported);

    return UNITY_END();
  }
}
