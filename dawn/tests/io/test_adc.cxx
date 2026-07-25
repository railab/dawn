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
#include "dawn/porting/adc.hxx"
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

//***************************************************************************
// Description: consecutive fetches keep the channel order - every fetch
// drains one whole conversion, nothing is left in the FIFO to rotate it.
//***************************************************************************

static void test_io_adc_fetch_repeat_order()
{
  CDescObject desc0(g_cfg_adc_fetch0);
  CIOAdcFetch adc0(desc0);
  io_sdata_t<int32_t, 32> data0;
  int i;

  TEST_ASSERT_EQUAL(OK, adc0.configure());
  TEST_ASSERT_EQUAL(OK, adc0.init());

  for (i = 0; i < 3; i++)
    {
      TEST_ASSERT_EQUAL(OK, adc0.getData(data0, 1));
      TEST_ASSERT_EQUAL(0, data0(0));
      TEST_ASSERT_EQUAL(1, data0(1));
      TEST_ASSERT_EQUAL(31, data0(31));
    }
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

//***************************************************************************
// Description: a read length that is a multiple of 5 is trimmed by one
// sample so the driver returns plain int32 records, others pass through;
// zero or a partial sample is rejected (0).
//***************************************************************************

static void test_io_adc_read_len_quirk()
{
  TEST_ASSERT_EQUAL(16, adc_read_len(20));
  TEST_ASSERT_EQUAL(36, adc_read_len(40));
  TEST_ASSERT_EQUAL(16, adc_read_len(16));
  TEST_ASSERT_EQUAL(12, adc_read_len(12));
  TEST_ASSERT_EQUAL(4, adc_read_len(4));
  TEST_ASSERT_EQUAL(0, adc_read_len(0));
  TEST_ASSERT_EQUAL(0, adc_read_len(6));
}

extern "C"
{
  int test_io_adc()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_adc_read_len_quirk);

#ifdef CONFIG_DAWN_IO_ADC_FETCH
    DAWN_RUN_TEST(test_io_adc_fetch_init);
    DAWN_RUN_TEST(test_io_adc_fetch_repeat_order);
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
