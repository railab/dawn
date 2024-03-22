// dawn/tests/proto/test_handler.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/handler.hxx"
#include "dawn/proto/handler.hxx"
#include "test_common.hxx"
#include "test_protomockcommon.hxx"

using namespace dawn;

//***************************************************************************
// Description: configuration with proto mock objects
//***************************************************************************

static uint32_t g_cfg_userfacotry[] = {
  // Header

  CDescriptor::DAWN_DESCRIPTOR_HDR, // Magic
  3,                                // Size

  // Object 1

  CProtoMockCommon::objectId(0),
  0,

  // Object 2

  CProtoMockCommon::objectId(2),
  0,

  // Object 3

  CProtoMockCommon::objectId(3),
  0,

  // Check sum

  CDescriptor::DAWN_DESCRIPTOR_FOOT,
  0xdeadbeef};

// Build a CProtoHandler initialised from g_cfg_userfacotry with a spy
// factory.  Returns the handler, descriptor, IO stub, and trace via local
// references; caller can introspect them.

#define PROTO_HANDLER_FIXTURE                                       \
  CProtoHandler proto;                                              \
  MockTrace trace = {};                                             \
  CProtoMockFactorySpy factory(&trace);                             \
  CDescriptor desc;                                                 \
  CIOHandler io;                                                    \
  desc.loadBin(g_cfg_userfacotry, sizeof(g_cfg_userfacotry), true); \
  TEST_ASSERT_EQUAL(OK, proto.init(desc, &io, &factory))

//***************************************************************************
// Description: init() invokes the user proto factory once per descriptor
// entry.
//***************************************************************************

static void test_proto_handler_user_factory_create_count()
{
  PROTO_HANDLER_FIXTURE;

  ASSERT_CALLS(trace, MOCK_EVENT_CREATE, 3);
}

//***************************************************************************
// Description: factory create events arrive in descriptor order with the
// matching object ids.
//***************************************************************************

static void test_proto_handler_user_factory_create_order()
{
  PROTO_HANDLER_FIXTURE;

  const mock_event_e expected_order[] = {MOCK_EVENT_CREATE, MOCK_EVENT_CREATE, MOCK_EVENT_CREATE};
  ASSERT_CALL_ORDER(trace, expected_order);

  ASSERT_CALL_AT(trace, 0, MOCK_EVENT_CREATE, static_cast<int>(CProtoMockCommon::objectId(0)));
  ASSERT_CALL_AT(trace, 1, MOCK_EVENT_CREATE, static_cast<int>(CProtoMockCommon::objectId(2)));
  ASSERT_CALL_AT(trace, 2, MOCK_EVENT_CREATE, static_cast<int>(CProtoMockCommon::objectId(3)));
}

//***************************************************************************
// Description: getProto() returns a non-null pointer for every object id
// declared in the descriptor.
//***************************************************************************

static void test_proto_handler_get_proto_for_known_ids()
{
  PROTO_HANDLER_FIXTURE;
  SObjectId::UObjectId id;

  id.v = CProtoMockCommon::objectId(0);
  TEST_ASSERT_NOT_NULL(proto.getProto(id));

  id.v = CProtoMockCommon::objectId(2);
  TEST_ASSERT_NOT_NULL(proto.getProto(id));

  id.v = CProtoMockCommon::objectId(3);
  TEST_ASSERT_NOT_NULL(proto.getProto(id));
}

extern "C"
{
  int test_proto_handler()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_proto_handler_user_factory_create_count);
    DAWN_RUN_TEST(test_proto_handler_user_factory_create_order);
    DAWN_RUN_TEST(test_proto_handler_get_proto_for_known_ids);

    return UNITY_END();
  }
}
