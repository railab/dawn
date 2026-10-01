// dawn/tests/proto/test_nxscope_dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/proto/nxscope/dummy.hxx"
#include "test_common.hxx"

#include <cstring>

#include <logging/nxscope/nxscope.h>

using namespace dawn;

static constexpr auto NXSCOPE_DUMMYIO1 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 0);
static constexpr auto NXSCOPE_DUMMYIO2 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 2);
static constexpr auto NXSCOPE_DUMMYIO3 = CIODummyNotify::objectId(SObjectId::DTYPE_INT32, false, 3);

static uint32_t g_cfg_dummy1[] = {
  NXSCOPE_DUMMYIO1,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_dummy2[] = {
  NXSCOPE_DUMMYIO2,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_cfg_dummy3[] = {
  NXSCOPE_DUMMYIO3,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_INT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  5000,
};

static uint32_t g_bin_nxscope_dummy[] = {
  CProtoNxscopeDummy::objectId(0),
  1,
  CProtoNxscopeDummy::cfgIdIOBind2(3),
  NXSCOPE_DUMMYIO1,
  0x00000061,
  0x00000000,
  0x00000000,
  NXSCOPE_DUMMYIO2,
  0x00000062,
  0x00000000,
  0x00000000,
  NXSCOPE_DUMMYIO3,
  0x00000063,
  0x00000000,
  0x00000000,
};

// Configure + init three CIODummyNotify dummies, configure + bind the
// nxscope dummy proto, init + start.  Caller stops nxscope and notifier.

#define NXSCOPE_DUMMY_FIXTURE                          \
  CDescObject descv1(g_cfg_dummy1);                    \
  CIODummyNotify dummy1(descv1);                       \
  CDescObject descv2(g_cfg_dummy2);                    \
  CIODummyNotify dummy2(descv2);                       \
  CDescObject descv3(g_cfg_dummy3);                    \
  CIODummyNotify dummy3(descv3);                       \
  CDescObject desc(g_bin_nxscope_dummy);               \
  CProtoNxscopeDummy nxscope(desc);                    \
  CIONotifier notifier;                                \
  TEST_ASSERT_EQUAL(OK, nxscope.configure());          \
  TEST_ASSERT_EQUAL(OK, dummy1.configure());           \
  TEST_ASSERT_EQUAL(OK, dummy1.init());                \
  TEST_ASSERT_EQUAL(OK, dummy2.configure());           \
  TEST_ASSERT_EQUAL(OK, dummy2.init());                \
  TEST_ASSERT_EQUAL(OK, dummy3.configure());           \
  TEST_ASSERT_EQUAL(OK, dummy3.init());                \
  dummy1.bindNotifier(&notifier);                      \
  dummy2.bindNotifier(&notifier);                      \
  dummy3.bindNotifier(&notifier);                      \
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO1, &dummy1); \
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO2, &dummy2); \
  nxscope.setObjectMapItem(NXSCOPE_DUMMYIO3, &dummy3); \
  TEST_ASSERT_EQUAL(OK, nxscope.init())

//***************************************************************************
// Description: nxscope dummy proto reports hasThread() false before init.
//***************************************************************************

static void test_proto_nxscope_dummy_idle_no_thread()
{
  NXSCOPE_DUMMY_FIXTURE;

  TEST_ASSERT_FALSE(nxscope.hasThread());
}

//***************************************************************************
// Description: nxscope dummy proto runs through start -> hasThread -> stop.
//***************************************************************************

static void test_proto_nxscope_dummy_lifecycle()
{
  NXSCOPE_DUMMY_FIXTURE;

  TEST_ASSERT_EQUAL(OK, nxscope.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_TRUE(nxscope.hasThread());

  TEST_ASSERT_EQUAL(OK, nxscope.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_FALSE(nxscope.hasThread());
}

//***************************************************************************
// nxscope_put_samples(): a block put must leave the stream buffer exactly as
// the same samples put one by one with nxscope_put_vXXXX().
//***************************************************************************

struct SNxsPair
{
  struct nxscope_intf_s intf;
  struct nxscope_dummy_cfg_s icfg;
  struct nxscope_proto_s proto;
  struct nxscope_s one;
  struct nxscope_s blk;
};

static char g_nxs_name[] = "ch";

static void nxsInit(struct nxscope_s *s, SNxsPair &p, size_t len)
{
  struct nxscope_cfg_s cfg;

  std::memset(&cfg, 0, sizeof(cfg));
  cfg.intf_cmd = &p.intf;
  cfg.intf_stream = &p.intf;
  cfg.proto_cmd = &p.proto;
  cfg.proto_stream = &p.proto;
  cfg.channels = 4;
  cfg.streambuf_len = len;
  cfg.rxbuf_len = 32;
  cfg.cribuf_len = 64;
  TEST_ASSERT_EQUAL(OK, nxscope_init(s, &cfg));

  TEST_ASSERT_EQUAL(OK, nxscope_chan_init(s, 0, g_nxs_name, NXSCOPE_TYPE_INT16, 1, 0));
  TEST_ASSERT_EQUAL(OK, nxscope_chan_init(s, 1, g_nxs_name, NXSCOPE_TYPE_INT32, 3, 0));
  TEST_ASSERT_EQUAL(OK, nxscope_chan_init(s, 2, g_nxs_name, NXSCOPE_TYPE_FLOAT, 1, 0));
  TEST_ASSERT_EQUAL(OK, nxscope_chan_init(s, 3, g_nxs_name, NXSCOPE_TYPE_UINT8, 2, 0));
  TEST_ASSERT_EQUAL(OK, nxscope_chan_all_en(s, true));
  TEST_ASSERT_EQUAL(OK, nxscope_stream_start(s, true));
}

static void nxsPairInit(SNxsPair &p, size_t len)
{
  std::memset(&p, 0, sizeof(p));
  TEST_ASSERT_EQUAL(OK, nxscope_dummy_init(&p.intf, &p.icfg));
  TEST_ASSERT_EQUAL(OK, nxscope_proto_ser_init(&p.proto, nullptr));
  nxsInit(&p.one, p, len);
  nxsInit(&p.blk, p, len);
}

static void nxsPairDeinit(SNxsPair &p)
{
  nxscope_deinit(&p.one);
  nxscope_deinit(&p.blk);
  nxscope_proto_ser_deinit(&p.proto);
  nxscope_dummy_deinit(&p.intf);
}

static void nxsAssertSame(SNxsPair &p)
{
  TEST_ASSERT_EQUAL(p.one.stream_i, p.blk.stream_i);
  TEST_ASSERT_EQUAL_MEMORY(p.one.streambuf, p.blk.streambuf, p.one.stream_i);
}

//***************************************************************************
// Description: block put of int16 points matches per-sample puts and the
// documented wire layout (channel id, little-endian value).
//***************************************************************************

static void test_nxscope_put_samples_int16()
{
  SNxsPair p;
  int16_t v[40];
  size_t i;
  size_t base;

  nxsPairInit(p, 512);
  base = p.one.stream_i;

  for (i = 0; i < 40; i++)
    {
      v[i] = static_cast<int16_t>(0x1234 + 0x101 * i);
      TEST_ASSERT_EQUAL(OK, nxscope_put_vint16(&p.one, 0, &v[i], 1));
    }

  TEST_ASSERT_EQUAL(OK,
                    nxscope_put_samples(&p.blk, NXSCOPE_TYPE_INT16, 0, v, 1, 40, sizeof(int16_t)));
  nxsAssertSame(p);

  TEST_ASSERT_EQUAL(base + 40 * 3, p.blk.stream_i);
  TEST_ASSERT_EQUAL_HEX8(0x00, p.blk.streambuf[base]);
  TEST_ASSERT_EQUAL_HEX8(0x34, p.blk.streambuf[base + 1]);
  TEST_ASSERT_EQUAL_HEX8(0x12, p.blk.streambuf[base + 2]);

  nxsPairDeinit(p);
}

//***************************************************************************
// Description: vectors read through a padded stride (e.g. timestamped
// batches) and several channels interleaved keep the per-sample layout.
//***************************************************************************

static void test_nxscope_put_samples_stride_mixed()
{
  struct SPad
  {
    int32_t v[3];
    uint32_t ts;
  };

  SNxsPair p;
  SPad pad[6];
  float f[5];
  uint8_t u[4][2];
  size_t i;

  nxsPairInit(p, 512);

  for (i = 0; i < 6; i++)
    {
      pad[i].v[0] = static_cast<int32_t>(i);
      pad[i].v[1] = -static_cast<int32_t>(i * 1000);
      pad[i].v[2] = 0x7f000000 + static_cast<int32_t>(i);
      pad[i].ts = 0xdeadbeef;
    }

  for (i = 0; i < 5; i++)
    {
      f[i] = 1.5f * i;
    }

  for (i = 0; i < 4; i++)
    {
      u[i][0] = static_cast<uint8_t>(i);
      u[i][1] = static_cast<uint8_t>(0xf0 + i);
    }

  for (i = 0; i < 6; i++)
    {
      TEST_ASSERT_EQUAL(OK, nxscope_put_vint32(&p.one, 1, pad[i].v, 3));
    }

  for (i = 0; i < 5; i++)
    {
      TEST_ASSERT_EQUAL(OK, nxscope_put_vfloat(&p.one, 2, &f[i], 1));
    }

  for (i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL(OK, nxscope_put_vuint8(&p.one, 3, u[i], 2));
    }

  TEST_ASSERT_EQUAL(
    OK, nxscope_put_samples(&p.blk, NXSCOPE_TYPE_INT32, 1, pad[0].v, 3, 6, sizeof(SPad)));
  TEST_ASSERT_EQUAL(OK, nxscope_put_samples(&p.blk, NXSCOPE_TYPE_FLOAT, 2, f, 1, 5, sizeof(float)));
  TEST_ASSERT_EQUAL(OK, nxscope_put_samples(&p.blk, NXSCOPE_TYPE_UINT8, 3, u, 2, 4, 2));
  nxsAssertSame(p);

  nxsPairDeinit(p);
}

//***************************************************************************
// Description: the channel divider decimates inside a block and its counter
// carries over to the next block, as with per-sample puts.
//***************************************************************************

static void test_nxscope_put_samples_divider()
{
  SNxsPair p;
  int16_t v[12];
  size_t i;
  size_t base;

  nxsPairInit(p, 512);
  base = p.blk.stream_i;
  TEST_ASSERT_EQUAL(OK, nxscope_chan_div(&p.one, 0, 2));
  TEST_ASSERT_EQUAL(OK, nxscope_chan_div(&p.blk, 0, 2));

  for (i = 0; i < 12; i++)
    {
      v[i] = static_cast<int16_t>(i);
      nxscope_put_vint16(&p.one, 0, &v[i], 1);
    }

  TEST_ASSERT_EQUAL(OK,
                    nxscope_put_samples(&p.blk, NXSCOPE_TYPE_INT16, 0, v, 1, 7, sizeof(int16_t)));
  TEST_ASSERT_EQUAL(
    OK, nxscope_put_samples(&p.blk, NXSCOPE_TYPE_INT16, 0, &v[7], 1, 5, sizeof(int16_t)));
  nxsAssertSame(p);
  TEST_ASSERT_EQUAL(p.one.cntr[0], p.blk.cntr[0]);

  // Divider 2 keeps every third sample: v[2], v[5], v[8], v[11]

  TEST_ASSERT_EQUAL(base + 4 * 3, p.blk.stream_i);
  TEST_ASSERT_EQUAL_HEX8(2, p.blk.streambuf[base + 1]);

  nxsPairDeinit(p);
}

//***************************************************************************
// Description: on a full stream buffer the block put stores what fits,
// flags the overflow and returns -ENOBUFS, like per-sample puts.
//***************************************************************************

static void test_nxscope_put_samples_overflow()
{
  SNxsPair p;
  int16_t v[50];
  size_t i;
  int ret = OK;

  nxsPairInit(p, 64);

  for (i = 0; i < 50; i++)
    {
      v[i] = static_cast<int16_t>(i);
      if (ret == OK)
        {
          ret = nxscope_put_vint16(&p.one, 0, &v[i], 1);
        }
    }

  TEST_ASSERT_EQUAL(-ENOBUFS, ret);
  TEST_ASSERT_EQUAL(-ENOBUFS,
                    nxscope_put_samples(&p.blk, NXSCOPE_TYPE_INT16, 0, v, 1, 50, sizeof(int16_t)));
  nxsAssertSame(p);
  TEST_ASSERT_NOT_EQUAL(0, p.blk.streambuf[p.blk.proto_stream->hdrlen]);

  nxsPairDeinit(p);
}

//***************************************************************************
// Description: a stopped stream or a disabled channel stores nothing and
// returns -EAGAIN.
//***************************************************************************

static void test_nxscope_put_samples_inactive()
{
  SNxsPair p;
  int16_t v[4] = {1, 2, 3, 4};
  size_t before;

  nxsPairInit(p, 512);
  before = p.blk.stream_i;

  TEST_ASSERT_EQUAL(OK, nxscope_chan_en(&p.blk, 0, false));
  TEST_ASSERT_EQUAL(-EAGAIN,
                    nxscope_put_samples(&p.blk, NXSCOPE_TYPE_INT16, 0, v, 1, 4, sizeof(int16_t)));
  TEST_ASSERT_EQUAL(before, p.blk.stream_i);

  TEST_ASSERT_EQUAL(OK, nxscope_chan_en(&p.blk, 0, true));
  TEST_ASSERT_EQUAL(OK, nxscope_stream_start(&p.blk, false));
  TEST_ASSERT_EQUAL(-EAGAIN,
                    nxscope_put_samples(&p.blk, NXSCOPE_TYPE_INT16, 0, v, 1, 4, sizeof(int16_t)));
  TEST_ASSERT_EQUAL(before, p.blk.stream_i);

  nxsPairDeinit(p);
}

extern "C"
{
  int test_proto_nxscope_dummy()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_proto_nxscope_dummy_idle_no_thread);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_lifecycle);
    DAWN_RUN_TEST(test_nxscope_put_samples_int16);
    DAWN_RUN_TEST(test_nxscope_put_samples_stride_mixed);
    DAWN_RUN_TEST(test_nxscope_put_samples_divider);
    DAWN_RUN_TEST(test_nxscope_put_samples_overflow);
    DAWN_RUN_TEST(test_nxscope_put_samples_inactive);

    return UNITY_END();
  }
}
