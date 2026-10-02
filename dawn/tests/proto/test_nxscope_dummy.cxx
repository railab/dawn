// dawn/tests/proto/test_proto_nxscope_dummy_dummy.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include <cstring>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <unistd.h>

#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/proto/nxscope/dummy.hxx"
#include "logging/nxscope/nxscope.h"
#include "test_common.hxx"

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

static void test_proto_nxscope_dummy_put_samples_int16()
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

static void test_proto_nxscope_dummy_put_samples_stride_mixed()
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

static void test_proto_nxscope_dummy_put_samples_divider()
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

static void test_proto_nxscope_dummy_put_samples_overflow()
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

static void test_proto_nxscope_dummy_put_samples_inactive()
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

//***************************************************************************
// nxscope send paths: frames must reach the interface whole and valid when
// the interface writes short, fails, or for critical channels.
//***************************************************************************

static uint8_t g_cap[8][4096];
static size_t g_caplen[8];
static int g_capn;
static int g_capfail;
static int g_cappart;

static int capSend(struct nxscope_intf_s *intf, uint8_t *buff, int len)
{
  (void)intf;

  if (g_capfail > 0)
    {
      g_capfail--;
      return -EAGAIN;
    }

  if (g_cappart > 0 && g_cappart < len)
    {
      len = g_cappart;
      g_cappart = 0;
    }

  if (g_capn < 8)
    {
      std::memcpy(g_cap[g_capn], buff, len);
      g_caplen[g_capn] = len;
    }

  g_capn++;
  return len;
}

static int capRecv(struct nxscope_intf_s *intf, uint8_t *buff, int len)
{
  (void)intf;
  (void)buff;
  (void)len;
  return 0;
}

static struct nxscope_intf_ops_s g_capops = {capSend, capRecv};

static void nxsCapInit(struct nxscope_s *s,
                       struct nxscope_intf_s *intf,
                       struct nxscope_proto_s *proto,
                       size_t len,
                       size_t crilen)
{
  struct nxscope_cfg_s cfg;
  union nxscope_chinfo_type_u u;

  std::memset(&cfg, 0, sizeof(cfg));
  cfg.intf_cmd = intf;
  cfg.intf_stream = intf;
  cfg.proto_cmd = proto;
  cfg.proto_stream = proto;
  cfg.channels = 2;
  cfg.streambuf_len = len;
  cfg.rxbuf_len = 32;
  cfg.cribuf_len = crilen;
  TEST_ASSERT_EQUAL(OK, nxscope_init(s, &cfg));
  TEST_ASSERT_EQUAL(OK, nxscope_chan_init(s, 0, g_nxs_name, NXSCOPE_TYPE_INT16, 1, 0));
  u.u8 = 0;
  u.s.dtype = NXSCOPE_TYPE_UINT8;
  u.s.cri = 1;
  TEST_ASSERT_EQUAL(OK, nxscope_chan_init(s, 1, g_nxs_name, u.u8, 1, 0));
  TEST_ASSERT_EQUAL(OK, nxscope_chan_all_en(s, true));
  TEST_ASSERT_EQUAL(OK, nxscope_stream_start(s, true));
}

// Assert buf holds exactly one valid stream frame; return its data

static uint8_t *nxsAssertFrame(struct nxscope_proto_s *proto,
                               uint8_t *buf,
                               size_t len,
                               size_t *dlen)
{
  struct nxscope_frame_s frame;

  TEST_ASSERT_EQUAL(OK, proto->ops->frame_get(proto, buf, len, &frame));
  TEST_ASSERT_EQUAL(NXSCOPE_HDRID_STREAM, frame.id);
  TEST_ASSERT_EQUAL(len, frame.drop);
  *dlen = frame.dlen;
  return frame.data;
}

struct SFifoRx
{
  int fd;
  uint8_t buf[4096];
  size_t len;
  size_t want;
};

static void *fifoReader(void *arg)
{
  SFifoRx *rx = static_cast<SFifoRx *>(arg);
  int tries;
  ssize_t n;

  for (tries = 0; tries < 2000 && rx->len < rx->want; tries++)
    {
      n = read(rx->fd, &rx->buf[rx->len], std::min<size_t>(256, sizeof(rx->buf) - rx->len));
      if (n > 0)
        {
          rx->len += n;
        }

      usleep(1000);
    }

  return nullptr;
}

//***************************************************************************
// Description: a frame larger than the free space of a non-blocking serial
// interface goes out in parts over repeated flushes and arrives whole.
//***************************************************************************

static void test_proto_nxscope_dummy_ser_short_write()
{
  static char path[] = "/dev/nxsfifo";
  struct nxscope_intf_s cintf;
  struct nxscope_intf_s sintf;
  struct nxscope_ser_cfg_s scfg;
  struct nxscope_proto_s proto;
  struct nxscope_s ref;
  struct nxscope_s s;
  static SFifoRx rx;
  pthread_t th;
  int16_t v;
  int i;

  std::memset(&cintf, 0, sizeof(cintf));
  cintf.ops = &g_capops;
  cintf.initialized = true;
  TEST_ASSERT_EQUAL(OK, nxscope_proto_ser_init(&proto, nullptr));
  nxsCapInit(&ref, &cintf, &proto, 4096, 64);

  unlink(path);
  TEST_ASSERT_EQUAL(OK, mkfifo(path, 0666));
  rx.fd = open(path, O_RDONLY | O_NONBLOCK);
  TEST_ASSERT_TRUE(rx.fd >= 0);
  std::memset(&scfg, 0, sizeof(scfg));
  scfg.path = path;
  scfg.nonblock = true;
  TEST_ASSERT_EQUAL(OK, nxscope_ser_init(&sintf, &scfg));
  nxsCapInit(&s, &sintf, &proto, 4096, 64);

  for (i = 0; i < 900; i++)
    {
      v = static_cast<int16_t>(i * 7);
      TEST_ASSERT_EQUAL(OK, nxscope_put_int16(&ref, 0, v));
      TEST_ASSERT_EQUAL(OK, nxscope_put_int16(&s, 0, v));
    }

  g_capn = 0;
  g_capfail = 0;
  TEST_ASSERT_TRUE(nxscope_stream(&ref) >= 0);
  TEST_ASSERT_EQUAL(1, g_capn);

  rx.len = 0;
  rx.want = g_caplen[0];
  TEST_ASSERT_EQUAL(0, pthread_create(&th, nullptr, fifoReader, &rx));

  // Flush as a user loop does: a partial send is completed later

  for (i = 0; i < 200 && nxscope_stream(&s) < 0; i++)
    {
      usleep(1000);
    }

  TEST_ASSERT_TRUE(i > 0);
  TEST_ASSERT_TRUE(i < 200);
  pthread_join(th, nullptr);

  TEST_ASSERT_EQUAL(g_caplen[0], rx.len);
  TEST_ASSERT_EQUAL_MEMORY(g_cap[0], rx.buf, g_caplen[0]);

  nxscope_deinit(&s);
  nxscope_deinit(&ref);
  nxscope_ser_deinit(&sintf);
  close(rx.fd);
  unlink(path);
}

//***************************************************************************
// Description: a stream frame whose send failed is resent unchanged;
// samples put meanwhile are dropped and flagged in the next frame.
//***************************************************************************

static void test_proto_nxscope_dummy_stream_retry()
{
  struct nxscope_intf_s cintf;
  struct nxscope_proto_s proto;
  struct nxscope_s s;
  static uint8_t first[64];
  size_t firstlen;
  size_t dlen;
  uint8_t *data;
  int i;

  std::memset(&cintf, 0, sizeof(cintf));
  cintf.ops = &g_capops;
  cintf.initialized = true;
  TEST_ASSERT_EQUAL(OK, nxscope_proto_ser_init(&proto, nullptr));
  nxsCapInit(&s, &cintf, &proto, 512, 64);

  for (i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL(OK, nxscope_put_int16(&s, 0, static_cast<int16_t>(i)));
    }

  g_capn = 0;
  g_capfail = 1;
  TEST_ASSERT_TRUE(nxscope_stream(&s) < 0);
  firstlen = s.stream_i;
  std::memcpy(first, s.streambuf, firstlen);
  nxsAssertFrame(&proto, first, firstlen, &dlen);

  // Puts while the frame waits for its retry

  for (i = 0; i < 3; i++)
    {
      nxscope_put_int16(&s, 0, static_cast<int16_t>(100 + i));
    }

  TEST_ASSERT_TRUE(nxscope_stream(&s) >= 0);
  TEST_ASSERT_EQUAL(1, g_capn);
  TEST_ASSERT_EQUAL(firstlen, g_caplen[0]);
  TEST_ASSERT_EQUAL_MEMORY(first, g_cap[0], firstlen);

  TEST_ASSERT_EQUAL(OK, nxscope_put_int16(&s, 0, 200));
  TEST_ASSERT_TRUE(nxscope_stream(&s) >= 0);
  TEST_ASSERT_EQUAL(2, g_capn);
  data = nxsAssertFrame(&proto, g_cap[1], g_caplen[1], &dlen);
  TEST_ASSERT_EQUAL(NXSCOPE_STREAM_FLAGS_OVERFLOW, data[0] & NXSCOPE_STREAM_FLAGS_OVERFLOW);
  TEST_ASSERT_EQUAL(1 + 3, dlen);

  nxscope_deinit(&s);
}

//***************************************************************************
// Description: a critical channel sends its sample at once as one valid
// stream frame, and a failed critical send leaves the stream intact.
//***************************************************************************

static void test_proto_nxscope_dummy_critical_channel()
{
  struct nxscope_intf_s cintf;
  struct nxscope_proto_s proto;
  struct nxscope_s s;
  size_t before;
  size_t dlen;
  uint8_t *data;

  std::memset(&cintf, 0, sizeof(cintf));
  cintf.ops = &g_capops;
  cintf.initialized = true;
  TEST_ASSERT_EQUAL(OK, nxscope_proto_ser_init(&proto, nullptr));

  // Frame: header 4, flags 1, channel 1, uint8 1, CRC 2

  nxsCapInit(&s, &cintf, &proto, 512, 9);
  before = s.stream_i;

  g_capn = 0;
  g_capfail = 0;
  TEST_ASSERT_EQUAL(OK, nxscope_put_uint8(&s, 1, 0x42));
  TEST_ASSERT_EQUAL(before, s.stream_i);
  TEST_ASSERT_EQUAL(1, g_capn);
  data = nxsAssertFrame(&proto, g_cap[0], g_caplen[0], &dlen);
  TEST_ASSERT_EQUAL(3, dlen);
  TEST_ASSERT_EQUAL_HEX8(0x00, data[0]);
  TEST_ASSERT_EQUAL_HEX8(0x01, data[1]);
  TEST_ASSERT_EQUAL_HEX8(0x42, data[2]);

  // A failed critical send must not mark the stream frame for retry

  g_capfail = 1;
  TEST_ASSERT_TRUE(nxscope_put_uint8(&s, 1, 0x43) < 0);
  TEST_ASSERT_EQUAL(OK, nxscope_put_int16(&s, 0, 0x1234));
  g_capn = 0;
  TEST_ASSERT_TRUE(nxscope_stream(&s) >= 0);
  TEST_ASSERT_EQUAL(1, g_capn);
  data = nxsAssertFrame(&proto, g_cap[0], g_caplen[0], &dlen);
  TEST_ASSERT_EQUAL(1 + 3, dlen);

  nxscope_deinit(&s);
}

//***************************************************************************
// Description: a stream frame sent only in part is completed by the retry,
// so the interface carries one whole frame.
//***************************************************************************

static void test_proto_nxscope_dummy_stream_partial()
{
  struct nxscope_intf_s cintf;
  struct nxscope_proto_s proto;
  struct nxscope_s s;
  static uint8_t whole[64];
  size_t len;
  size_t dlen;
  int i;

  std::memset(&cintf, 0, sizeof(cintf));
  cintf.ops = &g_capops;
  cintf.initialized = true;
  TEST_ASSERT_EQUAL(OK, nxscope_proto_ser_init(&proto, nullptr));
  nxsCapInit(&s, &cintf, &proto, 512, 64);

  for (i = 0; i < 4; i++)
    {
      TEST_ASSERT_EQUAL(OK, nxscope_put_int16(&s, 0, static_cast<int16_t>(i)));
    }

  g_capn = 0;
  g_capfail = 0;
  g_cappart = 7;
  TEST_ASSERT_TRUE(nxscope_stream(&s) < 0);
  TEST_ASSERT_TRUE(nxscope_stream(&s) >= 0);
  TEST_ASSERT_EQUAL(2, g_capn);

  len = g_caplen[0] + g_caplen[1];
  TEST_ASSERT_EQUAL(7, g_caplen[0]);
  std::memcpy(whole, g_cap[0], g_caplen[0]);
  std::memcpy(&whole[g_caplen[0]], g_cap[1], g_caplen[1]);
  nxsAssertFrame(&proto, whole, len, &dlen);
  TEST_ASSERT_EQUAL(1 + 4 * 3, dlen);

  nxscope_deinit(&s);
}

extern "C"
{
  int test_proto_nxscope_dummy()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_proto_nxscope_dummy_idle_no_thread);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_lifecycle);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_put_samples_int16);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_put_samples_stride_mixed);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_put_samples_divider);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_put_samples_overflow);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_put_samples_inactive);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_ser_short_write);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_stream_retry);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_critical_channel);
    DAWN_RUN_TEST(test_proto_nxscope_dummy_stream_partial);

    return UNITY_END();
  }
}
