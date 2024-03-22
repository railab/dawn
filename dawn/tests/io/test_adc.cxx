// dawn/tests/io/test_adc.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <poll.h>
#include <unistd.h>

#ifdef CONFIG_DAWN_IO_ADC_FETCH
#  include "dawn/io/adc_fetch.hxx"
#endif
#ifdef CONFIG_DAWN_IO_ADC_SYNC
#  include "dawn/io/adc_sync.hxx"
#endif
#ifdef CONFIG_DAWN_IO_ADC_STREAM
#  include "dawn/io/adc_stream.hxx"
#endif
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

#ifdef CONFIG_DAWN_IO_ADC_FETCH
static uint32_t g_cfg_adc_fetch0[] = {
  CIOAdcFetch::objectId(false, 0),
  1,
  CIOCommon::cfgIdDevno(),
  0,
};

static uint32_t g_cfg_adc_fetch1[] = {
  CIOAdcFetch::objectId(false, 2),
  1,
  CIOCommon::cfgIdDevno(),
  1,
};

static uint32_t g_cfg_adc_fetch2[] = {
  CIOAdcFetch::objectId(false, 3),
  1,
  CIOCommon::cfgIdDevno(),
  2,
};
#endif

#ifdef CONFIG_DAWN_IO_ADC_SYNC
static uint32_t g_cfg_adc_sync[] = {
  CIOAdcSync::objectId(false, 4),
  2,
  CIOCommon::cfgIdDevno(),
  0,
  CIOAdcSync::cfgId(true, SObjectId::DTYPE_UINT32, 1, CIOAdcSync::IO_ADC_SYNC_CFG_TRIGGER_FREQ),
  50,
};
#endif

#ifdef CONFIG_DAWN_IO_ADC_STREAM
static uint32_t g_cfg_adc_stream[] = {
  CIOAdcStream::objectId(false, 5),
  2,
  CIOCommon::cfgIdDevno(),
  1,
  CIOAdcStream::cfgId(true, SObjectId::DTYPE_UINT32, 1, CIOAdcStream::IO_ADC_STREAM_CFG_BATCH_SIZE),
  4,
};
#endif

#ifdef CONFIG_DAWN_IO_ADC_FETCH

//***************************************************************************
// Description: ADC fetch reads deterministic samples from each device.
//***************************************************************************

static void test_io_adc_fetch_init()
{
  CDescObject desc0(g_cfg_adc_fetch0);
  CDescObject desc1(g_cfg_adc_fetch1);
  CDescObject desc2(g_cfg_adc_fetch2);
  CIOAdcFetch adc0(desc0);
  CIOAdcFetch adc1(desc1);
  CIOAdcFetch adc2(desc2);
  io_sdata_t<int32_t, 32> data0;
  io_sdata_t<int32_t, 32> data1;
  io_sdata_t<int32_t, 32> data2;

  TEST_ASSERT_EQUAL(OK, adc0.configure());
  TEST_ASSERT_EQUAL(OK, adc0.init());
  TEST_ASSERT_EQUAL(OK, adc1.configure());
  TEST_ASSERT_EQUAL(OK, adc1.init());
  TEST_ASSERT_EQUAL(OK, adc2.configure());
  TEST_ASSERT_EQUAL(OK, adc2.init());

  TEST_ASSERT_EQUAL(OK, adc0.getData(data0, 1));
  TEST_ASSERT_EQUAL(0, data0(0));
  TEST_ASSERT_EQUAL(1, data0(1));
  TEST_ASSERT_EQUAL(2, data0(2));
  TEST_ASSERT_EQUAL(3, data0(3));

  TEST_ASSERT_EQUAL(OK, adc1.getData(data1, 1));
  TEST_ASSERT_EQUAL(1, data1(0));
  TEST_ASSERT_EQUAL(2, data1(1));
  TEST_ASSERT_EQUAL(3, data1(2));
  TEST_ASSERT_EQUAL(4, data1(3));

  TEST_ASSERT_EQUAL(OK, adc2.getData(data2, 1));
  TEST_ASSERT_EQUAL(2, data2(0));
  TEST_ASSERT_EQUAL(3, data2(1));
  TEST_ASSERT_EQUAL(4, data2(2));
  TEST_ASSERT_EQUAL(5, data2(3));
}
#endif

#ifdef CONFIG_DAWN_IO_ADC_SYNC

//***************************************************************************
// Description: ADC sync signals readiness and returns one sample batch.
//***************************************************************************

static void test_io_adc_sync_init()
{
  CDescObject desc(g_cfg_adc_sync);
  CIOAdcSync adc(desc);
  io_sdata_t<int32_t, 32> data;
  struct pollfd pfd;

  TEST_ASSERT_EQUAL(OK, adc.configure());
  TEST_ASSERT_EQUAL(OK, adc.init());
  TEST_ASSERT_EQUAL(OK, adc.start());

  pfd.fd = adc.getFd();
  pfd.events = POLLIN;
  pfd.revents = 0;

  TEST_ASSERT_EQUAL(1, poll(&pfd, 1, 200));
  TEST_ASSERT(pfd.revents & POLLIN);

  TEST_ASSERT_EQUAL(OK, adc.getData(data, 1));
  TEST_ASSERT_EQUAL(0, data(0));
  TEST_ASSERT_EQUAL(1, data(1));
  TEST_ASSERT_EQUAL(2, data(2));
  TEST_ASSERT_EQUAL(3, data(3));

  TEST_ASSERT_EQUAL(OK, adc.stop());
}
#endif

#ifdef CONFIG_DAWN_IO_ADC_STREAM

//***************************************************************************
// Description: ADC stream signals readiness and returns a configured batch.
//***************************************************************************

static void test_io_adc_stream_init()
{
  CDescObject desc(g_cfg_adc_stream);
  CIOAdcStream adc(desc);
  io_sdata_t<int32_t, 32, 4> data;
  struct pollfd pfd;

  TEST_ASSERT_EQUAL(OK, adc.configure());
  TEST_ASSERT_EQUAL(OK, adc.init());
  TEST_ASSERT_EQUAL(OK, adc.start());

  pfd.fd = adc.getFd();
  pfd.events = POLLIN;
  pfd.revents = 0;

  TEST_ASSERT_EQUAL(1, poll(&pfd, 1, 200));
  TEST_ASSERT(pfd.revents & POLLIN);

  usleep(100000);

  TEST_ASSERT_EQUAL(OK, adc.getData(data, 4));
  TEST_ASSERT_EQUAL(1, data(0, 0));
  TEST_ASSERT_EQUAL(2, data(1, 0));
  TEST_ASSERT_EQUAL(1, data(0, 3));
  TEST_ASSERT_EQUAL(2, data(1, 3));

  TEST_ASSERT_EQUAL(OK, adc.stop());
}
#endif

extern "C"
{
  int test_io_adc()
  {
    UNITY_BEGIN();

#ifdef CONFIG_DAWN_IO_ADC_FETCH
    DAWN_RUN_TEST(test_io_adc_fetch_init);
#endif
#ifdef CONFIG_DAWN_IO_ADC_SYNC
    DAWN_RUN_TEST(test_io_adc_sync_init);
#endif
#ifdef CONFIG_DAWN_IO_ADC_STREAM
    DAWN_RUN_TEST(test_io_adc_stream_init);
#endif

    return UNITY_END();
  }
}
