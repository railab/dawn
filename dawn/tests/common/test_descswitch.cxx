// dawn/tests/common/test_descswitch.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/dev/descswitch.hxx"
#include "test_common.hxx"

using namespace dawn;

static void reset_switch_state()
{
  CDescSwitch::clear();
  CDescSwitch::setActiveSlot(0);
}

//***************************************************************************
// Description: descriptor switch initial state
//***************************************************************************

static void test_descswitch_init()
{
  reset_switch_state();

  TEST_ASSERT_EQUAL(false, CDescSwitch::isSwitchRequested());
  TEST_ASSERT_EQUAL(0u, CDescSwitch::getActiveSlot());
}

//***************************************************************************
// Description: descriptor switch clear
//***************************************************************************

static void test_descswitch_request_clear()
{
  reset_switch_state();

  CDescSwitch::requestSwitch(1);
  TEST_ASSERT_EQUAL(true, CDescSwitch::isSwitchRequested());
  TEST_ASSERT_EQUAL(1u, CDescSwitch::getSwitchSlot());

  CDescSwitch::clear();
  TEST_ASSERT_EQUAL(false, CDescSwitch::isSwitchRequested());
  TEST_ASSERT_EQUAL(1u, CDescSwitch::getSwitchSlot());
}

//***************************************************************************
// Description: active descriptor slot can be set and read back.
//***************************************************************************

static void test_descswitch_active_slot_roundtrip()
{
  reset_switch_state();

  CDescSwitch::setActiveSlot(2);
  TEST_ASSERT_EQUAL(2u, CDescSwitch::getActiveSlot());
}

//***************************************************************************
// Description: later switch requests overwrite earlier pending slots.
//***************************************************************************

static void test_descswitch_overwrite_request()
{
  reset_switch_state();

  CDescSwitch::requestSwitch(1);
  CDescSwitch::requestSwitch(2);

  TEST_ASSERT_EQUAL(true, CDescSwitch::isSwitchRequested());
  TEST_ASSERT_EQUAL(2u, CDescSwitch::getSwitchSlot());
}

extern "C"
{
  int test_common_descswitch()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_descswitch_init);
    DAWN_RUN_TEST(test_descswitch_request_clear);
    DAWN_RUN_TEST(test_descswitch_active_slot_roundtrip);
    DAWN_RUN_TEST(test_descswitch_overwrite_request);

    return UNITY_END();
  }
}
