// dawn/tests/io/test_control.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/control.hxx"
#include "dawn/io/sdata.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Name: CIOMockRunnable
//
// Description: runnable IO mock tracks start/stop state for control tests.
//
//***************************************************************************

class CIOMockRunnable : public dawn::CIOCommon
{
public:
  explicit CIOMockRunnable(CDescObject &desc)
    : dawn::CIOCommon(desc)
  {
    running = false;
  }

  ~CIOMockRunnable() override
  {
    deinit();
  }

  int doStart() override
  {
    running = true;
    return OK;
  }

  int doStop() override
  {
    running = false;
    return OK;
  }

  bool hasThread() const override
  {
    return running;
  }

  int getDataImpl(IODataCmn &data, size_t len) override
  {
    UNUSED(data);
    UNUSED(len);
    return OK;
  }

  int setDataImpl(IODataCmn &data) override
  {
    UNUSED(data);
    return OK;
  }

  size_t getDataSize() const override
  {
    return sizeof(uint8_t);
  }

  size_t getDataDim() const override
  {
    return 1;
  }

  bool isRead() const override
  {
    return true;
  }

  bool isWrite() const override
  {
    return true;
  }

  bool isNotify() const override
  {
    return false;
  }

  bool isBatch() const override
  {
    return false;
  }

  constexpr static SObjectId::ObjectId objectId(uint16_t inst)
  {
    return SObjectId::objectId(SObjectId::OBJTYPE_IO, 501, SObjectId::DTYPE_UINT8, 0, inst);
  }

private:
  bool running;
};

static constexpr auto MOCK_RUNNABLE_IO = CIOMockRunnable::objectId(0);

//***************************************************************************
// Description: control IO with both start and stop allowed
//***************************************************************************

static uint32_t g_cfg_ctrl1[] = {
  CIOControl::objectId(0),
  2,
  CIOControl::cfgIdAllocObj(1),
  MOCK_RUNNABLE_IO,
  CIOControl::cfgIdAllowed(),
  CIOControl::CTRL_ALLOW_START | CIOControl::CTRL_ALLOW_STOP,
};

//***************************************************************************
// Description: control IO with stop-only allowed
//***************************************************************************

static uint32_t g_cfg_ctrl2[] = {
  CIOControl::objectId(2),
  2,
  CIOControl::cfgIdAllocObj(1),
  MOCK_RUNNABLE_IO,
  CIOControl::cfgIdAllowed(),
  CIOControl::CTRL_ALLOW_STOP,
};

//***************************************************************************
// Description: control IO with no targets configured
//***************************************************************************

static uint32_t g_cfg_ctrl3[] = {
  CIOControl::objectId(3),
  1,
  CIOControl::cfgIdAllowed(),
  CIOControl::CTRL_ALLOW_START | CIOControl::CTRL_ALLOW_STOP,
};

//***************************************************************************
// Description: mock runnable target
//***************************************************************************

static uint32_t g_cfg_mock[] = {MOCK_RUNNABLE_IO, 0};

//***************************************************************************
// Description: test start/stop commands and state read
//***************************************************************************

static void test_io_control_start_stop()
{
  CDescObject desc(g_cfg_ctrl1);
  CIOControl ctrl(desc);

  CDescObject desc_mock(g_cfg_mock);
  CIOMockRunnable mock(desc_mock);

  io_sdata_t<uint8_t, 1> data;
  int ret;

  // Initialize

  TEST_ASSERT_EQUAL(OK, ctrl.configure());
  TEST_ASSERT_EQUAL(OK, ctrl.init());
  TEST_ASSERT_EQUAL(OK, mock.init());

  // Check IDs populated by configure()

  TEST_ASSERT_EQUAL(1u, ctrl.ids.size());
  TEST_ASSERT_EQUAL(MOCK_RUNNABLE_IO, ctrl.ids[0]);

  // Bind target

  ret = ctrl.bind(&mock);
  TEST_ASSERT_EQUAL(OK, ret);

  // Initially stopped

  TEST_ASSERT_EQUAL(OK, ctrl.getData(data, 1));
  TEST_ASSERT_EQUAL(CObject::STATE_STOPPED, data(0));

  // Start command (write 1)

  data(0) = 1;
  TEST_ASSERT_EQUAL(OK, ctrl.setData(data));
  TEST_ASSERT(mock.hasThread());

  // State read should now return running

  TEST_ASSERT_EQUAL(OK, ctrl.getData(data, 1));
  TEST_ASSERT_EQUAL(CObject::STATE_RUNNING, data(0));

  // Stop command (write 0)

  data(0) = 0;
  TEST_ASSERT_EQUAL(OK, ctrl.setData(data));
  TEST_ASSERT(!mock.hasThread());

  // State read should return stopped again

  TEST_ASSERT_EQUAL(OK, ctrl.getData(data, 1));
  TEST_ASSERT_EQUAL(CObject::STATE_STOPPED, data(0));
}

//***************************************************************************
// Description: test disallowed command returns -EACCES
//***************************************************************************

static void test_io_control_not_allowed()
{
  CDescObject desc(g_cfg_ctrl2);
  CIOControl ctrl(desc);

  CDescObject desc_mock(g_cfg_mock);
  CIOMockRunnable mock(desc_mock);

  io_sdata_t<uint8_t, 1> data;
  int ret;

  // Initialize

  TEST_ASSERT_EQUAL(OK, ctrl.configure());
  TEST_ASSERT_EQUAL(OK, ctrl.init());
  TEST_ASSERT_EQUAL(OK, mock.init());

  // Bind target

  ret = ctrl.bind(&mock);
  TEST_ASSERT_EQUAL(OK, ret);

  // Start command must be rejected (only STOP is allowed)

  data(0) = 1;
  TEST_ASSERT_EQUAL(-EACCES, ctrl.setData(data));
  TEST_ASSERT(!mock.hasThread());

  // Stop command must be accepted

  data(0) = 0;
  TEST_ASSERT_EQUAL(OK, ctrl.setData(data));
}

//***************************************************************************
// Description: test invalid command returns -EINVAL
//***************************************************************************

static void test_io_control_invalid_cmd()
{
  CDescObject desc(g_cfg_ctrl1);
  CIOControl ctrl(desc);

  CDescObject desc_mock(g_cfg_mock);
  CIOMockRunnable mock(desc_mock);

  io_sdata_t<uint8_t, 1> data;
  int ret;

  // Initialize

  TEST_ASSERT_EQUAL(OK, ctrl.configure());
  TEST_ASSERT_EQUAL(OK, ctrl.init());
  TEST_ASSERT_EQUAL(OK, mock.init());

  ret = ctrl.bind(&mock);
  TEST_ASSERT_EQUAL(OK, ret);

  // Command 2 is invalid

  data(0) = 2;
  TEST_ASSERT_EQUAL(-EINVAL, ctrl.setData(data));
}

//***************************************************************************
// Description: test getDataImpl with no targets returns -ENOENT
//***************************************************************************

static void test_io_control_no_targets()
{
  CDescObject desc(g_cfg_ctrl3);
  CIOControl ctrl(desc);

  io_sdata_t<uint8_t, 1> data;

  // Initialize but do not bind any targets

  TEST_ASSERT_EQUAL(OK, ctrl.configure());
  TEST_ASSERT_EQUAL(OK, ctrl.init());

  // Reading state with no targets must fail

  TEST_ASSERT_EQUAL(-ENOENT, ctrl.getData(data, 1));
}

extern "C"
{
  int test_io_control()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_control_start_stop);
    DAWN_RUN_TEST(test_io_control_not_allowed);
    DAWN_RUN_TEST(test_io_control_invalid_cmd);
    DAWN_RUN_TEST(test_io_control_no_targets);

    return UNITY_END();
  }
}
