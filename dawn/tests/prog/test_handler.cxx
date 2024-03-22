// dawn/tests/prog/test_handler.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/handler.hxx"
#include "dawn/prog/handler.hxx"
#include "test_common.hxx"
#include "test_progmockcommon.hxx"

using namespace dawn;

static uint32_t g_cfg_user_factory[] = {
  // Header

  CDescriptor::DAWN_DESCRIPTOR_HDR, // Magic
  3,                                // Size

  // Object 1

  CProgMockCommon::objectId(0),
  0,

  // Object 2

  CProgMockCommon::objectId(2),
  0,

  // Object 3

  CProgMockCommon::objectId(3),
  0,

  // Check sum

  CDescriptor::DAWN_DESCRIPTOR_FOOT,
  0xdeadbeef};

// Build a CProgHandler initialised from g_cfg_user_factory with a spy
// factory.

#define PROG_HANDLER_FIXTURE                                                                 \
  CProgHandler prog;                                                                         \
  CIOHandler io;                                                                             \
  MockTrace trace = {};                                                                      \
  CProgMockFactorySpy factory(&trace);                                                       \
  CDescriptor desc;                                                                          \
  TEST_ASSERT_EQUAL(OK, desc.loadBin(g_cfg_user_factory, sizeof(g_cfg_user_factory), true)); \
  TEST_ASSERT_EQUAL(OK, prog.init(desc, &io, &factory))

//***************************************************************************
// Description: prog handler init() invokes the user factory once per
// descriptor entry.
//***************************************************************************

static void test_prog_handler_user_factory_create_count()
{
  PROG_HANDLER_FIXTURE;

  ASSERT_CALLS(trace, MOCK_EVENT_CREATE, 3);
}

//***************************************************************************
// Description: factory create events arrive in descriptor order with the
// matching object ids.
//***************************************************************************

static void test_prog_handler_user_factory_create_order()
{
  PROG_HANDLER_FIXTURE;

  const mock_event_e expected_order[] = {MOCK_EVENT_CREATE, MOCK_EVENT_CREATE, MOCK_EVENT_CREATE};
  ASSERT_CALL_ORDER(trace, expected_order);

  ASSERT_CALL_AT(trace, 0, MOCK_EVENT_CREATE, static_cast<int>(CProgMockCommon::objectId(0)));
  ASSERT_CALL_AT(trace, 1, MOCK_EVENT_CREATE, static_cast<int>(CProgMockCommon::objectId(2)));
  ASSERT_CALL_AT(trace, 2, MOCK_EVENT_CREATE, static_cast<int>(CProgMockCommon::objectId(3)));
}

//***************************************************************************
// Description: getProg() returns a non-null pointer for every object id
// declared in the descriptor.
//***************************************************************************

static void test_prog_handler_get_prog_for_known_ids()
{
  PROG_HANDLER_FIXTURE;
  SObjectId::UObjectId id;

  id.v = CProgMockCommon::objectId(0);
  TEST_ASSERT_NOT_NULL(prog.getProg(id));

  id.v = CProgMockCommon::objectId(2);
  TEST_ASSERT_NOT_NULL(prog.getProg(id));

  id.v = CProgMockCommon::objectId(3);
  TEST_ASSERT_NOT_NULL(prog.getProg(id));
}

//***************************************************************************
// Description: getProg() returns null for an object id not in the
// descriptor.
//***************************************************************************

static void test_prog_handler_get_prog_unknown_id_null()
{
  PROG_HANDLER_FIXTURE;
  SObjectId::UObjectId id;

  id.v = CProgMockCommon::objectId(4);
  TEST_ASSERT_NULL(prog.getProg(id));
}

extern "C"
{
  int test_prog_handler()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_prog_handler_user_factory_create_count);
    DAWN_RUN_TEST(test_prog_handler_user_factory_create_order);
    DAWN_RUN_TEST(test_prog_handler_get_prog_for_known_ids);
    DAWN_RUN_TEST(test_prog_handler_get_prog_unknown_id_null);

    return UNITY_END();
  }
}
