// dawn/tests/prog/test_buffer.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dummy_notify.hxx"
#include "dawn/io/notifier.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/buffer.hxx"
#include "test_common.hxx"

using namespace dawn;

static constexpr auto BUFFER_SRC = CIODummyNotify::objectId(SObjectId::DTYPE_UINT32, false, 21);
static constexpr auto BUFFER_OUT = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 31);
static constexpr auto BUFFER_SEL = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 32);
static constexpr auto BUFFER_STAT = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 33);

static uint32_t g_cfg_buffer_src[] = {
  BUFFER_SRC,
  2,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  1000,
};

static uint32_t g_cfg_buffer_src_write_notify[] = {
  BUFFER_SRC,
  3,
  CIODummyNotify::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1),
  0,
  CIODummyNotify::cfgInterval(false),
  1000000,
  CIODummyNotify::cfgNotifyOnWrite(false),
  1,
};

static uint32_t g_cfg_buffer_out[] = {
  BUFFER_OUT,
  0,
};

static uint32_t g_cfg_buffer_out_chunk[] = {
  BUFFER_OUT,
  0,
};

static uint32_t g_cfg_buffer_sel[] = {
  BUFFER_SEL,
  0,
};

static uint32_t g_cfg_buffer_stat[] = {
  BUFFER_STAT,
  0,
};

static uint32_t g_bin_buffer_ring[] = {
  CProgBuffer::objectId(0),
  3,
  CProgBuffer::cfgIdIOBind(4),
  BUFFER_SRC,
  BUFFER_OUT,
  BUFFER_SEL,
  BUFFER_STAT,
  CProgBuffer::cfgIdDepth(),
  3,
  CProgBuffer::cfgIdFlags(),
  CProgBuffer::FLAG_AUTO_START,
};

static uint32_t g_bin_buffer_chunk[] = {
  CProgBuffer::objectId(1),
  4,
  CProgBuffer::cfgIdIOBind(4),
  BUFFER_SRC,
  BUFFER_OUT,
  BUFFER_SEL,
  BUFFER_STAT,
  CProgBuffer::cfgIdDepth(),
  6,
  CProgBuffer::cfgIdFlags(),
  CProgBuffer::FLAG_AUTO_START,
  CProgBuffer::cfgIdChunkSize(),
  4,
};

//***************************************************************************
// Description: ring buffer selects buffered samples and reports overflow state.
//***************************************************************************

static void test_prog_buffer_ring_select_and_overflow()
{
  CDescObject descSrc(g_cfg_buffer_src);
  CIODummyNotify src(descSrc);
  CDescObject descOut(g_cfg_buffer_out);
  CIOVirt out(descOut);
  CDescObject descSel(g_cfg_buffer_sel);
  CIOVirt sel(descSel);
  CDescObject descStat(g_cfg_buffer_stat);
  CIOVirt stat(descStat);
  CDescObject descProg(g_bin_buffer_ring);
  CProgBuffer prog(descProg);
  CIONotifier notifier;
  io_sdata_t<uint32_t, 1, 1> outData;
  io_sdata_t<uint32_t, 1, 1> selData;
  io_sdata_t<uint32_t, 8, 1> statData;
  uint32_t newest;
  uint32_t oldest;
  io_sdata_t<uint32_t, 1, 1> srcData;

  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, out.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, sel.init());
  TEST_ASSERT_EQUAL(OK, stat.init());
  TEST_ASSERT_EQUAL(OK, prog.configure());

  src.bindNotifier(&notifier);
  prog.setObjectMapItem(BUFFER_SRC, &src);
  prog.setObjectMapItem(BUFFER_OUT, &out);
  prog.setObjectMapItem(BUFFER_SEL, &sel);
  prog.setObjectMapItem(BUFFER_STAT, &stat);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, src.start());

  srcData(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);
  srcData(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);
  srcData(0) = 2;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);
  srcData(0) = 3;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);

  selData(0) = 0;
  TEST_ASSERT_EQUAL(OK, sel.setData(selData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  newest = outData(0);

  selData(0) = 2;
  TEST_ASSERT_EQUAL(OK, sel.setData(selData));
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  oldest = outData(0);
  TEST_ASSERT_EQUAL(2u, newest - oldest);

  TEST_ASSERT_EQUAL(OK, stat.getData(statData, 1));

  TEST_ASSERT_EQUAL(OK, src.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: buffer pause/resume triggers stop and restart sample capture.
//***************************************************************************

static void test_prog_buffer_trigger_pause_resume()
{
  CDescObject descSrc(g_cfg_buffer_src);
  CIODummyNotify src(descSrc);
  CDescObject descOut(g_cfg_buffer_out);
  CIOVirt out(descOut);
  CDescObject descSel(g_cfg_buffer_sel);
  CIOVirt sel(descSel);
  CDescObject descStat(g_cfg_buffer_stat);
  CIOVirt stat(descStat);
  CDescObject descProg(g_bin_buffer_ring);
  CProgBuffer prog(descProg);
  CIONotifier notifier;
  io_sdata_t<uint32_t, 1, 1> outData;
  io_sdata_t<uint32_t, 1, 1> selData;
  io_sdata_t<uint32_t, 8, 1> statData;
  uint32_t beforePause;
  uint32_t whilePaused;
  uint32_t afterResume;
  io_sdata_t<uint32_t, 1, 1> srcData;

  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, out.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, sel.init());
  TEST_ASSERT_EQUAL(OK, stat.init());
  TEST_ASSERT_EQUAL(OK, prog.configure());

  src.bindNotifier(&notifier);
  prog.setObjectMapItem(BUFFER_SRC, &src);
  prog.setObjectMapItem(BUFFER_OUT, &out);
  prog.setObjectMapItem(BUFFER_SEL, &sel);
  prog.setObjectMapItem(BUFFER_STAT, &stat);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, src.start());

  selData(0) = 0;
  TEST_ASSERT_EQUAL(OK, sel.setData(selData));
  srcData(0) = 0;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);
  srcData(0) = 1;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  beforePause = outData(0);

  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_TRIGGER2));
  srcData(0) = 2;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);
  srcData(0) = 3;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  whilePaused = outData(0);
  TEST_ASSERT_EQUAL(beforePause, whilePaused);

  TEST_ASSERT_EQUAL(OK, stat.getData(statData, 1));

  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_TRIGGER1));
  srcData(0) = 4;
  TEST_ASSERT_EQUAL(OK, src.setData(srcData));
  usleep(2000);
  TEST_ASSERT_EQUAL(OK, out.getData(outData, 1));
  afterResume = outData(0);
  TEST_ASSERT_GREATER_THAN(beforePause, afterResume);

  TEST_ASSERT_EQUAL(OK, src.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, prog.stop());
}

//***************************************************************************
// Description: chunk output pages multiple selected samples per read.
//***************************************************************************

static void test_prog_buffer_chunk_output()
{
  CDescObject descSrc(g_cfg_buffer_src_write_notify);
  CIODummyNotify src(descSrc);
  CDescObject descOut(g_cfg_buffer_out_chunk);
  CIOVirt out(descOut);
  CDescObject descSel(g_cfg_buffer_sel);
  CIOVirt sel(descSel);
  CDescObject descStat(g_cfg_buffer_stat);
  CIOVirt stat(descStat);
  CDescObject descProg(g_bin_buffer_chunk);
  CProgBuffer prog(descProg);
  CIONotifier notifier;
  io_sdata_t<uint32_t, 1, 1> selData;
  io_sdata_t<uint32_t, 8, 1> statData;
  io_sdata_t<uint32_t, 4, 1> chunkData;
  io_sdata_t<uint32_t, 1, 1> srcData;

  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, out.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, sel.init());
  TEST_ASSERT_EQUAL(OK, stat.init());
  TEST_ASSERT_EQUAL(OK, prog.configure());

  src.bindNotifier(&notifier);
  prog.setObjectMapItem(BUFFER_SRC, &src);
  prog.setObjectMapItem(BUFFER_OUT, &out);
  prog.setObjectMapItem(BUFFER_SEL, &sel);
  prog.setObjectMapItem(BUFFER_STAT, &stat);
  TEST_ASSERT_EQUAL(OK, prog.init());
  TEST_ASSERT_EQUAL(OK, prog.start());
  TEST_ASSERT_EQUAL(OK, notifier.start());
  TEST_ASSERT_EQUAL(OK, src.start());

  for (uint32_t i = 0; i < 6; i++)
    {
      srcData(0) = i;
      TEST_ASSERT_EQUAL(OK, src.setData(srcData));
      for (uint32_t retry = 0; retry < 100; retry++)
        {
          TEST_ASSERT_EQUAL(OK, stat.getData(statData, 1));
          if (statData(0) >= i + 1)
            {
              break;
            }
          usleep(1000);
        }
      TEST_ASSERT_EQUAL(i + 1, statData(0));
    }

  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_TRIGGER2));

  selData(0) = 0;
  TEST_ASSERT_EQUAL(OK, sel.setData(selData));
  TEST_ASSERT_EQUAL(OK, out.getData(chunkData, 1));
  TEST_ASSERT_EQUAL(5u, chunkData(0));
  TEST_ASSERT_EQUAL(4u, chunkData(1));
  TEST_ASSERT_EQUAL(3u, chunkData(2));
  TEST_ASSERT_EQUAL(2u, chunkData(3));

  selData(0) = 4;
  TEST_ASSERT_EQUAL(OK, sel.setData(selData));
  TEST_ASSERT_EQUAL(OK, out.getData(chunkData, 1));
  TEST_ASSERT_EQUAL(1u, chunkData(0));
  TEST_ASSERT_EQUAL(0u, chunkData(1));
  TEST_ASSERT_EQUAL(0u, chunkData(2));
  TEST_ASSERT_EQUAL(0u, chunkData(3));

  TEST_ASSERT_EQUAL(OK, src.stop());
  TEST_ASSERT_EQUAL(OK, notifier.stop());
  TEST_ASSERT_EQUAL(OK, prog.stop());
}

extern "C"
{
  int test_prog_buffer()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_prog_buffer_ring_select_and_overflow);
    DAWN_RUN_TEST(test_prog_buffer_trigger_pause_resume);
    DAWN_RUN_TEST(test_prog_buffer_chunk_output);
    return UNITY_END();
  }
}
