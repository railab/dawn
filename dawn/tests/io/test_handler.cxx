// dawn/tests/io/test_handler.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/io/handler.hxx"
#include "dawn/io/virt.hxx"
#include "test_common.hxx"
#include "test_iomockcommon.hxx"

using namespace dawn;

//***************************************************************************
// Description: configuration for user factory
//***************************************************************************

static uint32_t g_cfg_user_factory[] = {
  // Header

  CDescriptor::DAWN_DESCRIPTOR_HDR, // Magic
  3,                                // Size

  // Object 1

  CIOMockCommon::objectId(0),
  0,

  // Object 2

  CIOMockCommon::objectId(2),
  0,

  // Object 3

  CIOMockCommon::objectId(3),
  0,

  // Check sum

  CDescriptor::DAWN_DESCRIPTOR_FOOT,
  0xdeadbeef};

static uint32_t g_cfg_duplicate_ids[] = {
  CDescriptor::DAWN_DESCRIPTOR_HDR,
  2,

  CIOMockCommon::objectId(0),
  0,

  CIOMockCommon::objectId(0),
  0,

  CDescriptor::DAWN_DESCRIPTOR_FOOT,
  0xdeadbeef,
};

//***************************************************************************
// Description: io handler with user-defined factory
//***************************************************************************

static void test_io_handler_user_factory()
{
  CIOHandler io;
  MockTrace trace = {};
  CIOMockFactorySpy factory(&trace);
  CDescriptor desc;
  SObjectId::UObjectId id;
  CIOCommon *cmn = nullptr;
  int ret;

  // Load binary config

  ret = desc.loadBin(g_cfg_user_factory, sizeof(g_cfg_user_factory), true);
  TEST_ASSERT_EQUAL(OK, ret);

  // Init IO handler

  TEST_ASSERT_EQUAL(OK, io.init(desc, &factory));
  ASSERT_CALLS(trace, MOCK_EVENT_CREATE, 3);
  const mock_event_e expected_order[] = {MOCK_EVENT_CREATE, MOCK_EVENT_CREATE, MOCK_EVENT_CREATE};
  ASSERT_CALL_ORDER(trace, expected_order);
  ASSERT_CALL_AT(trace, 0, MOCK_EVENT_CREATE, static_cast<int>(CIOMockCommon::objectId(0)));
  ASSERT_CALL_AT(trace, 1, MOCK_EVENT_CREATE, static_cast<int>(CIOMockCommon::objectId(2)));
  ASSERT_CALL_AT(trace, 2, MOCK_EVENT_CREATE, static_cast<int>(CIOMockCommon::objectId(3)));

  // Initialize IO

  TEST_ASSERT_EQUAL(OK, io.initAll());

  // Get IO - valid ID

  id.v = CIOMockCommon::objectId(0);
  cmn = io.getIO(id);
  TEST_ASSERT_NOT_NULL(cmn);

  id.v = CIOMockCommon::objectId(2);
  cmn = io.getIO(id);
  TEST_ASSERT_NOT_NULL(cmn);

  id.v = CIOMockCommon::objectId(3);
  cmn = io.getIO(id);
  TEST_ASSERT_NOT_NULL(cmn);

  // Get IO - no valid ID

  id.v = CIOMockCommon::objectId(4);
  cmn = io.getIO(id);
  TEST_ASSERT_NULL(cmn);

  id.v = CIOMockCommon::objectId(5);
  cmn = io.getIO(id);
  TEST_ASSERT_NULL(cmn);

  id.v = CIOMockCommon::objectId(6);
  cmn = io.getIO(id);
  TEST_ASSERT_NULL(cmn);
}

//***************************************************************************
// Description: bind configs
//***************************************************************************

static void test_io_handler_bind_configs()
{
  TEST_IGNORE_MESSAGE("TODO");
  TEST_ASSERT(0);
}

//***************************************************************************
// Description: duplicate IO descriptor IDs are rejected during handler init.
//***************************************************************************

static void test_io_handler_duplicate_ids_fail()
{
  CIOHandler io;
  MockTrace trace = {};
  CIOMockFactorySpy factory(&trace);
  CDescriptor desc;
  int ret;

  ret = desc.loadBin(g_cfg_duplicate_ids, sizeof(g_cfg_duplicate_ids), true);
  TEST_ASSERT_EQUAL(OK, ret);

  ret = io.init(desc, &factory);
  TEST_ASSERT_EQUAL(-EEXIST, ret);
  ASSERT_CALLS(trace, MOCK_EVENT_CREATE, 2);
}

extern "C"
{
  int test_io_handler()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_io_handler_user_factory);
    DAWN_RUN_TEST(test_io_handler_duplicate_ids_fail);
    DAWN_RUN_TEST(test_io_handler_bind_configs);

    return UNITY_END();
  }
}
