// dawn/tests/io/test_encoder.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/encoder.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_encoder0[] = {
  CIOEncoder::objectId(false, 0),
  2,
  CIOCommon::cfgIdDevno(),
  0,
  CIOEncoder::cfgIdPosmax(),
  200,
};

//***************************************************************************
// Description: encoder IO reads changing position data and accepts reset.
//***************************************************************************

static void test_io_encoder_init()
{
  CDescObject desc0(g_cfg_encoder0);
  CIOEncoder encoder0(desc0);
  io_sdata_t<int32_t, 1> data0;
  int32_t prev;

  TEST_ASSERT_EQUAL(OK, encoder0.configure());
  TEST_ASSERT_EQUAL(OK, encoder0.init());

  TEST_ASSERT_EQUAL(OK, encoder0.getData(data0, 1));
  prev = data0(0);

  TEST_ASSERT_EQUAL(OK, encoder0.getData(data0, 1));
  TEST_ASSERT_TRUE(data0(0) != prev);

  TEST_ASSERT_EQUAL(OK, encoder0.trigger(CObject::CMD_RESET));
  TEST_ASSERT_EQUAL(OK, encoder0.getData(data0, 1));
}

extern "C"
{
  int test_io_encoder()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_encoder_init);

    return UNITY_END();
  }
}
