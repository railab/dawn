// dawn/tests/io/test_encoder_index.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/encoder_index.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

static uint32_t g_cfg_encoder_index0[] = {
  CIOEncoderIndex::objectId(false, 0),
  2,
  CIOCommon::cfgIdDevno(),
  1,
  CIOEncoderIndex::cfgIdPosmax(),
  200,
};

//***************************************************************************
// Description: encoder-index IO reads position, index, and direction data.
//***************************************************************************

static void test_io_encoder_index_init()
{
  CDescObject desc0(g_cfg_encoder_index0);
  CIOEncoderIndex encoder0(desc0);
  io_sdata_t<int32_t, 3> data0;

  TEST_ASSERT_EQUAL(OK, encoder0.configure());
  TEST_ASSERT_EQUAL(OK, encoder0.init());

  TEST_ASSERT_EQUAL(OK, encoder0.getData(data0, 1));
  TEST_ASSERT_TRUE(data0(0) >= 0);

  TEST_ASSERT_EQUAL(OK, encoder0.trigger(CObject::CMD_RESET));
  TEST_ASSERT_EQUAL(OK, encoder0.getData(data0, 1));
}

extern "C"
{
  int test_io_encoder_index()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_encoder_index_init);

    return UNITY_END();
  }
}
