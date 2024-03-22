// dawn/tests/io/test_trigger.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/sdata.hxx"
#include "dawn/io/trigger.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Name: CIOMockTriggerable
//
// Description: simple triggerable IO mock records the last command and count.
//
//***************************************************************************

class CIOMockTriggerable : public dawn::CIOCommon
{
public:
  explicit CIOMockTriggerable(CDescObject &desc)
    : dawn::CIOCommon(desc)
    , last_cmd(255)
    , trigger_count(0)
    , trigger_ret(OK)
  {
  }

  ~CIOMockTriggerable() override
  {
    deinit();
  }

  int trigger(uint8_t cmd) override
  {
    last_cmd = cmd;
    trigger_count += 1;
    return trigger_ret;
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

  uint8_t last_cmd;
  int trigger_count;
  int trigger_ret;
};

static constexpr auto MOCK_TRIGGERABLE_IO = CIOMockTriggerable::objectId(0);
static constexpr auto MOCK_TRIGGERABLE_IO2 = CIOMockTriggerable::objectId(2);

//***************************************************************************
// Description: trigger IO with TRIGGER1 allowed, 1 target
//***************************************************************************

static uint32_t g_cfg_trig1[] = {
  CIOTrigger::objectId(0),
  2,
  CIOTrigger::cfgIdAllocObj(1),
  MOCK_TRIGGERABLE_IO,
  CIOTrigger::cfgIdAllowed(),
  CIOTrigger::TRIG_ALLOW_TRIGGER1,
};

//***************************************************************************
// Description: trigger IO with all commands allowed, 1 target
//***************************************************************************

static uint32_t g_cfg_trig2[] = {
  CIOTrigger::objectId(2),
  2,
  CIOTrigger::cfgIdAllocObj(1),
  MOCK_TRIGGERABLE_IO,
  CIOTrigger::cfgIdAllowed(),
  CIOTrigger::TRIG_ALLOW_RESET | CIOTrigger::TRIG_ALLOW_TRIGGER1 | CIOTrigger::TRIG_ALLOW_TRIGGER2 |
    CIOTrigger::TRIG_ALLOW_TRIGGER3,
};

//***************************************************************************
// Description: trigger IO with no targets configured
//***************************************************************************

static uint32_t g_cfg_trig3[] = {
  CIOTrigger::objectId(3),
  1,
  CIOTrigger::cfgIdAllowed(),
  CIOTrigger::TRIG_ALLOW_TRIGGER1,
};

//***************************************************************************
// Description: trigger IO with 2 targets
//***************************************************************************

static uint32_t g_cfg_trig4[] = {
  CIOTrigger::objectId(4),
  2,
  CIOTrigger::cfgIdAllocObj(2),
  MOCK_TRIGGERABLE_IO,
  MOCK_TRIGGERABLE_IO2,
  CIOTrigger::cfgIdAllowed(),
  CIOTrigger::TRIG_ALLOW_TRIGGER1,
};

//***************************************************************************
// Description: mock triggerable targets
//***************************************************************************

static uint32_t g_cfg_mock1[] = {MOCK_TRIGGERABLE_IO, 0};
static uint32_t g_cfg_mock2[] = {MOCK_TRIGGERABLE_IO2, 0};

//***************************************************************************
// Test cases
//***************************************************************************

//***************************************************************************
// Description: test trigger dispatch to a single target
//***************************************************************************

static void test_io_trigger_dispatch()
{
  CDescObject desc(g_cfg_trig1);
  CIOTrigger trig(desc);

  CDescObject desc_mock(g_cfg_mock1);
  CIOMockTriggerable mock(desc_mock);

  io_sdata_t<uint8_t, 1> data;
  int ret;

  // Initialize

  TEST_ASSERT_EQUAL(OK, trig.configure());
  TEST_ASSERT_EQUAL(OK, trig.init());
  TEST_ASSERT_EQUAL(OK, mock.init());

  // Check IDs populated by configure()

  TEST_ASSERT_EQUAL(1u, trig.ids.size());
  TEST_ASSERT_EQUAL(MOCK_TRIGGERABLE_IO, trig.ids[0]);

  // Bind target

  ret = trig.bind(&mock);
  TEST_ASSERT_EQUAL(OK, ret);

  // Dispatch TRIGGER1

  data(0) = CObject::CMD_TRIGGER1;
  TEST_ASSERT_EQUAL(OK, trig.setData(data));
  TEST_ASSERT_EQUAL(CObject::CMD_TRIGGER1, mock.last_cmd);
  TEST_ASSERT_EQUAL(1, mock.trigger_count);

  // Dispatch again

  data(0) = CObject::CMD_TRIGGER1;
  TEST_ASSERT_EQUAL(OK, trig.setData(data));
  TEST_ASSERT_EQUAL(2, mock.trigger_count);
}

//***************************************************************************
// Description: test disallowed command returns -EACCES
//***************************************************************************

static void test_io_trigger_not_allowed()
{
  CDescObject desc(g_cfg_trig1);
  CIOTrigger trig(desc);

  CDescObject desc_mock(g_cfg_mock1);
  CIOMockTriggerable mock(desc_mock);

  io_sdata_t<uint8_t, 1> data;

  // Initialize and bind

  TEST_ASSERT_EQUAL(OK, trig.configure());
  TEST_ASSERT_EQUAL(OK, trig.init());
  TEST_ASSERT_EQUAL(OK, mock.init());

  TEST_ASSERT_EQUAL(OK, trig.bind(&mock));

  // CMD_RESET is not in allowed (only TRIGGER1 is)

  data(0) = CObject::CMD_RESET;
  TEST_ASSERT_EQUAL(-EACCES, trig.setData(data));
  TEST_ASSERT_EQUAL(0, mock.trigger_count);

  // CMD_TRIGGER1 is allowed

  data(0) = CObject::CMD_TRIGGER1;
  TEST_ASSERT_EQUAL(OK, trig.setData(data));
  TEST_ASSERT_EQUAL(1, mock.trigger_count);
}

//***************************************************************************
// Description: test invalid command returns -EINVAL
//***************************************************************************

static void test_io_trigger_invalid_cmd()
{
  CDescObject desc(g_cfg_trig2);
  CIOTrigger trig(desc);

  CDescObject desc_mock(g_cfg_mock1);
  CIOMockTriggerable mock(desc_mock);

  io_sdata_t<uint8_t, 1> data;

  // Initialize and bind

  TEST_ASSERT_EQUAL(OK, trig.configure());
  TEST_ASSERT_EQUAL(OK, trig.init());
  TEST_ASSERT_EQUAL(OK, mock.init());
  TEST_ASSERT_EQUAL(OK, trig.bind(&mock));

  // Command 4 is out of range

  data(0) = 4;
  TEST_ASSERT_EQUAL(-EINVAL, trig.setData(data));
  TEST_ASSERT_EQUAL(0, mock.trigger_count);
}

//***************************************************************************
// Description: test setData with no targets returns -ENOENT
//***************************************************************************

static void test_io_trigger_no_targets()
{
  CDescObject desc(g_cfg_trig3);
  CIOTrigger trig(desc);

  io_sdata_t<uint8_t, 1> data;

  // Initialize but do not bind any targets

  TEST_ASSERT_EQUAL(OK, trig.configure());
  TEST_ASSERT_EQUAL(OK, trig.init());

  // IDs list must be empty (no ALLOCOBJ in descriptor)

  TEST_ASSERT_EQUAL(0u, trig.ids.size());

  // Dispatching command with no targets must fail

  data(0) = CObject::CMD_TRIGGER1;
  TEST_ASSERT_EQUAL(-ENOENT, trig.setData(data));
}

//***************************************************************************
// Description: test trigger dispatch to multiple targets
//***************************************************************************

static void test_io_trigger_multi_target()
{
  CDescObject desc(g_cfg_trig4);
  CIOTrigger trig(desc);

  CDescObject desc_mock1(g_cfg_mock1);
  CDescObject desc_mock2(g_cfg_mock2);
  CIOMockTriggerable mock1(desc_mock1);
  CIOMockTriggerable mock2(desc_mock2);

  io_sdata_t<uint8_t, 1> data;

  // Initialize

  TEST_ASSERT_EQUAL(OK, trig.configure());
  TEST_ASSERT_EQUAL(OK, trig.init());
  TEST_ASSERT_EQUAL(OK, mock1.init());
  TEST_ASSERT_EQUAL(OK, mock2.init());

  // Check 2 IDs populated

  TEST_ASSERT_EQUAL(2u, trig.ids.size());

  // Bind both targets

  TEST_ASSERT_EQUAL(OK, trig.bind(&mock1));
  TEST_ASSERT_EQUAL(OK, trig.bind(&mock2));

  // Dispatch TRIGGER1 - both mocks must be triggered

  data(0) = CObject::CMD_TRIGGER1;
  TEST_ASSERT_EQUAL(OK, trig.setData(data));
  TEST_ASSERT_EQUAL(CObject::CMD_TRIGGER1, mock1.last_cmd);
  TEST_ASSERT_EQUAL(1, mock1.trigger_count);
  TEST_ASSERT_EQUAL(CObject::CMD_TRIGGER1, mock2.last_cmd);
  TEST_ASSERT_EQUAL(1, mock2.trigger_count);
}

extern "C"
{
  int test_io_trigger()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_trigger_dispatch);
    DAWN_RUN_TEST(test_io_trigger_not_allowed);
    DAWN_RUN_TEST(test_io_trigger_invalid_cmd);
    DAWN_RUN_TEST(test_io_trigger_no_targets);
    DAWN_RUN_TEST(test_io_trigger_multi_target);

    return UNITY_END();
  }
}
