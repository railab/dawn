// dawn/tests/prog/test_process.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/dummy.hxx"
#include "dawn/prog/process.hxx"
#include "test_process_common.hxx"

namespace dawn
{

//***************************************************************************
// Description: CProgProcess test implementation copies source data to output.
//***************************************************************************

class CProgProcessTestImpl : public CProgProcess
{
public:
  explicit CProgProcessTestImpl(CDescObject &desc)
    : CProgProcess(desc)
  {
  }

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(
      SObjectId::OBJTYPE_PROG, CProgCommon::PROG_CLASS_STATS_SUM, SObjectId::DTYPE_ANY, 0, inst);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgId(bool rw, uint8_t size, uint8_t id)
  {
    return SObjectCfg::objectCfg(SObjectId::OBJTYPE_PROG,
                                 CProgCommon::PROG_CLASS_STATS_SUM,
                                 SObjectId::DTYPE_ANY,
                                 rw,
                                 size,
                                 id);
  }

  constexpr static SObjectCfg::ObjectCfgId cfgIdIOBind()
  {
    return CProgProcessTestImpl::cfgId(false, 2, PROG_STATS_CFG_IOBIND);
  }

private:
  void handle(CIOCommon *output,
              io_ddata_t *data,
              io_ddata_t *ioData,
              io_ddata_t *outputData,
              bool &initsample) override
  {
    (void)ioData;
    (void)outputData;

    output->setData(*data);
    initsample = false;
  }
};
} // namespace dawn

using CTestProgProcessImpl = dawn::CProgProcessTestImpl;

DEFINE_PROCESS_BIN(g_bin_process_impl, CTestProgProcessImpl);

static constexpr auto PROCESS_DUMMY_OUT = CIODummy::objectId(SObjectId::DTYPE_INT32, false, 9);

static uint32_t g_cfg_process_dummy_out[] = {
  PROCESS_DUMMY_OUT,
  2,
  CIODummy::cfgIdDim(),
  10,
  CIODummy::cfgIdInitval(SObjectId::DTYPE_INT32, true, 10),
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
};

uint32_t g_bin_process_generic_output[] = {
  CTestProgProcessImpl::objectId(1),
  1,
  CTestProgProcessImpl::cfgIdIOBind(),
  PROCESS_DUMMYIO1,
  PROCESS_DUMMY_OUT,
};

//***************************************************************************
// Description: process programs support reset but not trigger1 by default.
//***************************************************************************

static void test_prog_process_trigger_api()
{
  CDescObject desc1(g_cfg_process_dummy1);
  CIODummyNotify src(desc1);
  CDescObject desc2(g_cfg_process_virt1);
  CIOVirt virt(desc2);
  CDescObject desc3(g_bin_process_impl);
  CTestProgProcessImpl prog(desc3);
  CIONotifier notifier;
  int ret;

  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, prog.configure());
  src.bindNotifier(&notifier);
  prog.setObjectMapItem(PROCESS_DUMMYIO1, &src);
  prog.setObjectMapItem(PROCESS_VIRTIO1, &virt);
  TEST_ASSERT_EQUAL(OK, prog.init());

  ret = prog.start();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = notifier.start();
  TEST_ASSERT_EQUAL(OK, ret);

  process_set_dummy_data(src, 0);

  TEST_ASSERT_EQUAL(-ENOTSUP, prog.trigger(CObject::CMD_TRIGGER1));
  TEST_ASSERT_EQUAL(OK, prog.trigger(CObject::CMD_RESET));

  ret = notifier.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = prog.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: process start/stop updates object state without a thread.
//***************************************************************************

static void test_prog_process_state()
{
  CDescObject desc1(g_cfg_process_dummy1);
  CIODummyNotify src(desc1);
  CDescObject desc2(g_cfg_process_virt1);
  CIOVirt virt(desc2);
  CDescObject desc3(g_bin_process_impl);
  CTestProgProcessImpl prog(desc3);
  CIONotifier notifier;
  int ret;

  TEST_ASSERT_EQUAL(CObject::STATE_STOPPED, prog.getState());

  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, prog.configure());
  src.bindNotifier(&notifier);
  prog.setObjectMapItem(PROCESS_DUMMYIO1, &src);
  prog.setObjectMapItem(PROCESS_VIRTIO1, &virt);
  TEST_ASSERT_EQUAL(OK, prog.init());

  ret = prog.start();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(CObject::STATE_RUNNING, prog.getState());
  TEST_ASSERT_EQUAL(false, prog.hasThread());

  ret = notifier.start();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = notifier.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = prog.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  TEST_ASSERT_EQUAL(CObject::STATE_STOPPED, prog.getState());
}

//***************************************************************************
// Description: stopped process ignores updates until restarted.
//***************************************************************************

static void test_prog_process_start_stop_updates()
{
  CDescObject desc1(g_cfg_process_dummy1);
  CIODummyNotify src(desc1);
  CDescObject desc2(g_cfg_process_virt1);
  CIOVirt virt(desc2);
  CDescObject desc3(g_bin_process_impl);
  CTestProgProcessImpl prog(desc3);
  CIONotifier notifier;
  io_sdata_t<uint32_t, 10, 1> out;
  uint32_t before_stop = 0;
  uint32_t after_stop = 0;
  uint32_t after_restart = 0;
  int ret;

  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, virt.init());
  TEST_ASSERT_EQUAL(OK, prog.configure());
  src.bindNotifier(&notifier);
  prog.setObjectMapItem(PROCESS_DUMMYIO1, &src);
  prog.setObjectMapItem(PROCESS_VIRTIO1, &virt);
  TEST_ASSERT_EQUAL(OK, prog.init());

  ret = prog.start();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = notifier.start();
  TEST_ASSERT_EQUAL(OK, ret);

  process_set_dummy_data(src, 0);
  TEST_ASSERT_EQUAL(OK, virt.getData(out, 1));
  before_stop = out(0);

  ret = prog.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  process_set_dummy_data(src, 1);
  TEST_ASSERT_EQUAL(OK, virt.getData(out, 1));
  after_stop = out(0);

  ret = prog.start();
  TEST_ASSERT_EQUAL(OK, ret);
  process_set_dummy_data(src, 2);
  TEST_ASSERT_EQUAL(OK, virt.getData(out, 1));
  after_restart = out(0);

  TEST_ASSERT_EQUAL(before_stop, after_stop);
  TEST_ASSERT_NOT_EQUAL(after_stop, after_restart);

  ret = notifier.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = prog.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

//***************************************************************************
// Description: process programs can write through a generic CIOCommon output.
//***************************************************************************

static void test_prog_process_generic_output()
{
  CDescObject desc1(g_cfg_process_dummy1);
  CIODummyNotify src(desc1);
  CDescObject desc2(g_cfg_process_dummy_out);
  CIODummy target(desc2);
  CDescObject desc3(g_bin_process_generic_output);
  CTestProgProcessImpl prog(desc3);
  CIONotifier notifier;
  io_sdata_t<int32_t, 10, 1> out;
  int ret;

  TEST_ASSERT_EQUAL(OK, src.configure());
  TEST_ASSERT_EQUAL(OK, src.init());
  TEST_ASSERT_EQUAL(OK, target.configure());
  TEST_ASSERT_EQUAL(OK, target.init());
  TEST_ASSERT_EQUAL(OK, prog.configure());
  src.bindNotifier(&notifier);
  prog.setObjectMapItem(PROCESS_DUMMYIO1, &src);
  prog.setObjectMapItem(PROCESS_DUMMY_OUT, &target);
  TEST_ASSERT_EQUAL(OK, prog.init());

  ret = prog.start();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = notifier.start();
  TEST_ASSERT_EQUAL(OK, ret);

  process_set_dummy_data(src, 4);
  TEST_ASSERT_EQUAL(OK, target.getData(out, 1));
  TEST_ASSERT_EQUAL(4, out(0));
  TEST_ASSERT_EQUAL(13, out(9));

  ret = notifier.stop();
  TEST_ASSERT_EQUAL(OK, ret);
  ret = prog.stop();
  TEST_ASSERT_EQUAL(OK, ret);
}

extern "C"
{
  int test_prog_process()
  {
    UNITY_BEGIN();
    DAWN_RUN_TEST(test_prog_process_trigger_api);
    DAWN_RUN_TEST(test_prog_process_state);
    DAWN_RUN_TEST(test_prog_process_start_stop_updates);
    DAWN_RUN_TEST(test_prog_process_generic_output);
    return UNITY_END();
  }
}
