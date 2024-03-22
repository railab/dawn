// dawn/tests/prog/test_sequencer.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dummy.hxx"
#include "dawn/io/sdata.hxx"
#include "dawn/io/virt.hxx"
#include "dawn/prog/sequencer.hxx"
#include "test_common.hxx"

#include <unistd.h>

using namespace dawn;

static constexpr auto SEQ_DUMMY_U8_1 = CIODummy::objectId(SObjectId::DTYPE_UINT8, false, 0);
static constexpr auto SEQ_DUMMY_U8_2 = CIODummy::objectId(SObjectId::DTYPE_UINT8, false, 1);
static constexpr auto SEQ_DUMMY_U32_1 = CIODummy::objectId(SObjectId::DTYPE_UINT32, false, 0);
static constexpr auto SEQ_VIRT_U32_1 = CIOVirt::objectId(SObjectId::DTYPE_UINT32, false, 200);

static uint32_t g_cfg_seq_dummy_u8_1[] = {
  SEQ_DUMMY_U8_1,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT8, true, 1),
  0,
};

static uint32_t g_cfg_seq_dummy_u8_2[] = {
  SEQ_DUMMY_U8_2,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT8, true, 1),
  0,
};

static uint32_t g_cfg_seq_dummy_u32_1[] = {
  SEQ_DUMMY_U32_1,
  1,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_UINT32, true, 1),
  0,
};

static uint32_t g_cfg_seq_virt_u32_1[] = {
  SEQ_VIRT_U32_1,
  0,
};

static uint32_t g_cfg_seq_prog_u8[] = {
  CProgSequencer::objectId(0),
  3,
  CProgSequencer::cfgIdTargets(1),
  SEQ_DUMMY_U8_1,
  CProgSequencer::cfgIdStates(4),
  0x12,
  20000,
  0x34,
  20000,
  CProgSequencer::cfgIdStartIndex(),
  0,
};

static uint32_t g_cfg_seq_prog_u8_slow[] = {
  CProgSequencer::objectId(3),
  3,
  CProgSequencer::cfgIdTargets(1),
  SEQ_DUMMY_U8_1,
  CProgSequencer::cfgIdStates(4),
  0x12,
  1000000,
  0x34,
  1000000,
  CProgSequencer::cfgIdStartIndex(),
  0,
};

static uint32_t g_cfg_seq_prog_type_mismatch[] = {
  CProgSequencer::objectId(1),
  2,
  CProgSequencer::cfgIdTargets(2),
  SEQ_DUMMY_U8_1,
  SEQ_DUMMY_U32_1,
  CProgSequencer::cfgIdStates(2),
  1,
  10000,
};

static uint32_t g_cfg_seq_prog_virt_u32[] = {
  CProgSequencer::objectId(2),
  3,
  CProgSequencer::cfgIdTargets(1),
  SEQ_VIRT_U32_1,
  CProgSequencer::cfgIdStates(4),
  0x12,
  20000,
  0x34,
  20000,
  CProgSequencer::cfgIdStartIndex(),
  0,
};

//***************************************************************************
// Description: a reset trigger restores the configured start state and writes
// it to the target immediately.
//***************************************************************************

static void test_prog_sequencer_reset_applies_start_state()
{
  CDescObject desc_io(g_cfg_seq_dummy_u8_1);
  CIODummy out(desc_io);
  CDescObject desc_seq(g_cfg_seq_prog_u8);
  CProgSequencer seq(desc_seq);
  io_sdata_t<uint8_t, 1, 1> outdata;

  TEST_ASSERT_EQUAL(OK, out.configure());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, seq.configure());

  seq.setObjectMapItem(SEQ_DUMMY_U8_1, &out);
  TEST_ASSERT_EQUAL(OK, seq.init());

  TEST_ASSERT_EQUAL(OK, seq.start());
  usleep(50000);
  TEST_ASSERT_EQUAL(OK, seq.stop());

  TEST_ASSERT_EQUAL(OK, seq.trigger(CObject::CMD_RESET));
  TEST_ASSERT_EQUAL(OK, out.getData(outdata, 1));
  TEST_ASSERT_EQUAL_UINT8(0x12, outdata(0));

  TEST_ASSERT_EQUAL(OK, seq.deinit());
  TEST_ASSERT_EQUAL(OK, out.deinit());
}

//***************************************************************************
// Description: sequencer rejects a target list when target data type/size
// does not match across all configured outputs.
//***************************************************************************

static void test_prog_sequencer_init_fails_on_target_type_mismatch()
{
  CDescObject desc_u8(g_cfg_seq_dummy_u8_2);
  CIODummy out_u8(desc_u8);
  CDescObject desc_u32(g_cfg_seq_dummy_u32_1);
  CIODummy out_u32(desc_u32);
  CDescObject desc_seq(g_cfg_seq_prog_type_mismatch);
  CProgSequencer seq(desc_seq);

  TEST_ASSERT_EQUAL(OK, out_u8.configure());
  TEST_ASSERT_EQUAL(OK, out_u32.configure());
  TEST_ASSERT_EQUAL(OK, out_u8.init());
  TEST_ASSERT_EQUAL(OK, out_u32.init());

  TEST_ASSERT_EQUAL(OK, seq.configure());
  seq.setObjectMapItem(SEQ_DUMMY_U8_1, &out_u8);
  seq.setObjectMapItem(SEQ_DUMMY_U32_1, &out_u32);
  TEST_ASSERT_EQUAL(-EINVAL, seq.init());

  TEST_ASSERT_EQUAL(OK, out_u8.deinit());
  TEST_ASSERT_EQUAL(OK, out_u32.deinit());
}

//***************************************************************************
// Description: runtime updates to start index and states are reloaded on
// reset and immediately affect the applied output value.
//***************************************************************************

static void test_prog_sequencer_runtime_cfg_reload_on_reset()
{
  CDescObject desc_io(g_cfg_seq_dummy_u8_1);
  CIODummy out(desc_io);
  CDescObject desc_seq(g_cfg_seq_prog_u8);
  CProgSequencer seq(desc_seq);
  io_sdata_t<uint8_t, 1, 1> outdata;
  uint32_t start_value = 1;
  uint32_t state_words[4] = {0x55, 10000, 0x66, 10000};

  TEST_ASSERT_EQUAL(OK, out.configure());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, seq.configure());

  seq.setObjectMapItem(SEQ_DUMMY_U8_1, &out);
  TEST_ASSERT_EQUAL(OK, seq.init());

  TEST_ASSERT_EQUAL(OK, seq.setObjConfig(CProgSequencer::cfgIdStartIndex(), &start_value, 1));
  TEST_ASSERT_EQUAL(OK, seq.setObjConfig(CProgSequencer::cfgIdStates(4), state_words, 4));

  TEST_ASSERT_EQUAL(OK, seq.trigger(CObject::CMD_RESET));
  TEST_ASSERT_EQUAL(OK, out.getData(outdata, 1));
  TEST_ASSERT_EQUAL_UINT8(0x66, outdata(0));

  TEST_ASSERT_EQUAL(OK, seq.deinit());
  TEST_ASSERT_EQUAL(OK, out.deinit());
}

//***************************************************************************
// Description: runtime configuration writes while stopped update the current
// state view immediately.
//***************************************************************************

static void test_prog_sequencer_runtime_cfg_applies_while_stopped()
{
  CDescObject desc_io(g_cfg_seq_dummy_u8_1);
  CIODummy out(desc_io);
  CDescObject desc_seq(g_cfg_seq_prog_u8);
  CProgSequencer seq(desc_seq);
  io_sdata_t<uint8_t, 1, 1> outdata;
  uint32_t start_value = 1;
  uint32_t state_words[4] = {0x55, 10000, 0x66, 10000};

  TEST_ASSERT_EQUAL(OK, out.configure());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, seq.configure());

  seq.setObjectMapItem(SEQ_DUMMY_U8_1, &out);
  TEST_ASSERT_EQUAL(OK, seq.init());

  TEST_ASSERT_EQUAL(OK, seq.setObjConfig(CProgSequencer::cfgIdStartIndex(), &start_value, 1));
  TEST_ASSERT_EQUAL(OK, seq.setObjConfig(CProgSequencer::cfgIdStates(4), state_words, 4));

  TEST_ASSERT_EQUAL(OK, out.getData(outdata, 1));
  TEST_ASSERT_EQUAL_UINT8(0x66, outdata(0));

  TEST_ASSERT_EQUAL(OK, seq.deinit());
  TEST_ASSERT_EQUAL(OK, out.deinit());
}

//***************************************************************************
// Description: runtime state updates while running refresh the current phase
// and do not force the sequencer back to a different phase.
//***************************************************************************

static void test_prog_sequencer_runtime_cfg_preserves_running_phase()
{
  CDescObject desc_io(g_cfg_seq_dummy_u8_1);
  CIODummy out(desc_io);
  CDescObject desc_seq(g_cfg_seq_prog_u8_slow);
  CProgSequencer seq(desc_seq);
  io_sdata_t<uint8_t, 1, 1> outdata;
  uint32_t state_words[4] = {0x55, 1000000, 0x66, 1000000};

  TEST_ASSERT_EQUAL(OK, out.configure());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, seq.configure());

  seq.setObjectMapItem(SEQ_DUMMY_U8_1, &out);
  TEST_ASSERT_EQUAL(OK, seq.init());
  TEST_ASSERT_EQUAL(OK, seq.start());

  for (int i = 0; i < 10; i++)
    {
      TEST_ASSERT_EQUAL(OK, out.getData(outdata, 1));
      if (outdata(0) == 0x12)
        {
          break;
        }

      usleep(1000);
    }

  TEST_ASSERT_EQUAL_UINT8(0x12, outdata(0));
  TEST_ASSERT_EQUAL(OK, seq.setObjConfig(CProgSequencer::cfgIdStates(4), state_words, 4));
  TEST_ASSERT_EQUAL(OK, out.getData(outdata, 1));
  TEST_ASSERT_EQUAL_UINT8(0x55, outdata(0));

  TEST_ASSERT_EQUAL(OK, seq.stop());
  TEST_ASSERT_EQUAL(OK, seq.deinit());
  TEST_ASSERT_EQUAL(OK, out.deinit());
}

//***************************************************************************
// Description: sequencer owns deferred virt targets and initializes them
// before applying state values.
//***************************************************************************

static void test_prog_sequencer_owns_virt_target()
{
  CDescObject desc_io(g_cfg_seq_virt_u32_1);
  CIOVirt out(desc_io);
  CDescObject desc_seq(g_cfg_seq_prog_virt_u32);
  CProgSequencer seq(desc_seq);
  io_sdata_t<uint32_t, 1, 1> outdata;

  TEST_ASSERT_EQUAL(OK, out.configure());
  TEST_ASSERT_EQUAL(OK, out.init());
  TEST_ASSERT_EQUAL(OK, seq.configure());

  seq.setObjectMapItem(SEQ_VIRT_U32_1, &out);
  TEST_ASSERT_EQUAL(OK, seq.init());

  TEST_ASSERT_EQUAL(OK, seq.trigger(CObject::CMD_RESET));
  TEST_ASSERT_EQUAL(OK, out.getData(outdata, 1));
  TEST_ASSERT_EQUAL(0x12, outdata(0));

  TEST_ASSERT_EQUAL(OK, seq.deinit());
  TEST_ASSERT_EQUAL(OK, out.deinit());
}

extern "C"
{
  int test_prog_sequencer()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_sequencer_reset_applies_start_state);
    DAWN_RUN_TEST(test_prog_sequencer_init_fails_on_target_type_mismatch);
    DAWN_RUN_TEST(test_prog_sequencer_runtime_cfg_reload_on_reset);
    DAWN_RUN_TEST(test_prog_sequencer_runtime_cfg_applies_while_stopped);
    DAWN_RUN_TEST(test_prog_sequencer_runtime_cfg_preserves_running_phase);
    DAWN_RUN_TEST(test_prog_sequencer_owns_virt_target);

    return UNITY_END();
  }
}
