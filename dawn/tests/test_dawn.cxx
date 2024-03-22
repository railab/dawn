// dawn/tests/test_dawn.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/dawn.hxx"
#include "test_common.hxx"
#include "test_iomockcommon.hxx"
#include "test_progmockcommon.hxx"
#include "test_protomockcommon.hxx"

using namespace dawn;

// hand made c++ descriptor

static uint32_t g_dawn_desc[] = {
  // Header

  CDescriptor::DAWN_DESCRIPTOR_HDR,
  11,

  CDescriptor::objectId(0),
  2,
  CDescriptor::cfgIdVersion(),
  0x00020001, // Some version

  CDescriptor::cfgIdString(4),
  0x64646362, //   Some string
  0x00686766, //
  0x00000000, //
  0x00000000, //

  // Dummy IO 1

  CIOMockCommon::objectId(0),
  0,

  // Dummy IO 2

  CIOMockCommon::objectId(2),
  0,

  // Dummy IO 3

  CIOMockCommon::objectId(3),
  0,

  // Dummy IO 4

  CIOMockCommon::objectId(4),
  0,

  // Dummy IO 5

  CIOMockCommon::objectId(5),
  0,

  // Prog 1

  CProgMockCommon::objectId(0),
  0,

  // Prog 2

  CProgMockCommon::objectId(2),
  0,

  // Prog 3

  CProgMockCommon::objectId(3),
  0,

  // Proto 1

  CProtoMockCommon::objectId(0),
  0,

  // Proto 2

  CProtoMockCommon::objectId(2),
  0,

  // Check sum

  CDescriptor::DAWN_DESCRIPTOR_FOOT,
  0xdeadbeef};

//***************************************************************************
// Test cases
//***************************************************************************

//***************************************************************************
// Description: test dawn with custom user-defined factories
//***************************************************************************

// Build a CDawn instance with three spy factories, run binCheckFill +
// load_descriptor + start.  Returns via out params so each test can inspect
// the recorded traces.

#define DAWN_USER_FACTORY_FIXTURE                                                     \
  MockTrace io_trace = {};                                                            \
  MockTrace prog_trace = {};                                                          \
  MockTrace proto_trace = {};                                                         \
  CIOMockFactorySpy io_factory(&io_trace);                                            \
  CProgMockFactorySpy prog_factory(&prog_trace);                                      \
  CProtoMockFactorySpy proto_factory(&proto_trace);                                   \
  CDawn dawn(&io_factory, &prog_factory, &proto_factory);                             \
  TEST_ASSERT_EQUAL(OK, CDescriptor::binCheckFill(g_dawn_desc, sizeof(g_dawn_desc))); \
  TEST_ASSERT_EQUAL(OK, dawn.load_descriptor(g_dawn_desc, sizeof(g_dawn_desc)));      \
  TEST_ASSERT_EQUAL(OK, dawn.start(true))

//***************************************************************************
// Description: load_descriptor + start with a 5-IO descriptor invokes the
// IO factory exactly 5 times.
//***************************************************************************

static void test_dawn_user_factory_io_create_count()
{
  DAWN_USER_FACTORY_FIXTURE;

  ASSERT_CALLS(io_trace, MOCK_EVENT_CREATE, 5);
}

//***************************************************************************
// Description: load_descriptor with 3 PROG entries invokes the prog
// factory exactly 3 times.
//***************************************************************************

static void test_dawn_user_factory_prog_create_count()
{
  DAWN_USER_FACTORY_FIXTURE;

  ASSERT_CALLS(prog_trace, MOCK_EVENT_CREATE, 3);
}

//***************************************************************************
// Description: load_descriptor with 2 PROTO entries invokes the proto
// factory exactly 2 times.
//***************************************************************************

static void test_dawn_user_factory_proto_create_count()
{
  DAWN_USER_FACTORY_FIXTURE;

  ASSERT_CALLS(proto_trace, MOCK_EVENT_CREATE, 2);
}

//***************************************************************************
// Description: factory create events arrive in the order the descriptor
// declares the objects (IO, PROG, PROTO each in declared order).
//***************************************************************************

static void test_dawn_user_factory_create_in_descriptor_order()
{
  DAWN_USER_FACTORY_FIXTURE;

  const mock_event_e expected_io_order[] = {
    MOCK_EVENT_CREATE, MOCK_EVENT_CREATE, MOCK_EVENT_CREATE, MOCK_EVENT_CREATE, MOCK_EVENT_CREATE};
  const mock_event_e expected_prog_order[] = {
    MOCK_EVENT_CREATE, MOCK_EVENT_CREATE, MOCK_EVENT_CREATE};
  const mock_event_e expected_proto_order[] = {MOCK_EVENT_CREATE, MOCK_EVENT_CREATE};
  ASSERT_CALL_ORDER(io_trace, expected_io_order);
  ASSERT_CALL_ORDER(prog_trace, expected_prog_order);
  ASSERT_CALL_ORDER(proto_trace, expected_proto_order);
}

//***************************************************************************
// Description: each factory create call carries the matching object id
// from the descriptor.
//***************************************************************************

static void test_dawn_user_factory_create_with_object_ids()
{
  DAWN_USER_FACTORY_FIXTURE;

  ASSERT_CALL_AT(io_trace, 0, MOCK_EVENT_CREATE, static_cast<int>(CIOMockCommon::objectId(0)));
  ASSERT_CALL_AT(io_trace, 4, MOCK_EVENT_CREATE, static_cast<int>(CIOMockCommon::objectId(5)));
  ASSERT_CALL_AT(prog_trace, 0, MOCK_EVENT_CREATE, static_cast<int>(CProgMockCommon::objectId(0)));
  ASSERT_CALL_AT(prog_trace, 2, MOCK_EVENT_CREATE, static_cast<int>(CProgMockCommon::objectId(3)));
  ASSERT_CALL_AT(
    proto_trace, 0, MOCK_EVENT_CREATE, static_cast<int>(CProtoMockCommon::objectId(0)));
  ASSERT_CALL_AT(
    proto_trace, 1, MOCK_EVENT_CREATE, static_cast<int>(CProtoMockCommon::objectId(2)));
}

extern "C"
{
  int test_run_dawn()
  {
    DAWN_TEST_SEPARATOR();
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_dawn_user_factory_io_create_count);
    DAWN_RUN_TEST(test_dawn_user_factory_prog_create_count);
    DAWN_RUN_TEST(test_dawn_user_factory_proto_create_count);
    DAWN_RUN_TEST(test_dawn_user_factory_create_in_descriptor_order);
    DAWN_RUN_TEST(test_dawn_user_factory_create_with_object_ids);

    return UNITY_END();
  }
}
