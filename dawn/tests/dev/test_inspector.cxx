// dawn/tests/dev/test_inspector.cxx
//
// SPDX-License-Identifier: Apache-2.0
//

#include "dawn/porting/config.hxx"

#include "dawn/dev/inspector.hxx"
#include "dawn/io/handler.hxx"
#include "dawn/prog/handler.hxx"
#include "dawn/proto/handler.hxx"
#include "test_common.hxx"

using namespace dawn;

//***************************************************************************
// Description: check inspector singleton implementation
//***************************************************************************

static void test_dev_inspector_singleton()
{
  CDevInspector *inst1 = CDevInspector::getInst();
  CDevInspector *inst2 = CDevInspector::getInst();

  // Singleton - same instance

  TEST_ASSERT(inst1 == inst2);
  TEST_ASSERT(inst1 != nullptr);

  // Clean up

  CDevInspector::destroy();
}

//***************************************************************************
// Description: check handler registration and retrieval
//***************************************************************************

static void test_dev_inspector_registration()
{
  CDevInspector *inspector = CDevInspector::getInst();

  // Initially no handlers registered

  TEST_ASSERT(inspector->getIOHandler() == nullptr);
  TEST_ASSERT(inspector->getProgHandler() == nullptr);
  TEST_ASSERT(inspector->getProtoHandler() == nullptr);

  TEST_ASSERT_FALSE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_IO));
  TEST_ASSERT_FALSE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_PROG));
  TEST_ASSERT_FALSE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_PROTO));

  // Create dummy handlers (we'll use const pointers to avoid init)

  CIOHandler io_handler;
  CProgHandler prog_handler;
  CProtoHandler proto_handler;

  // Register handlers

  inspector->registerIOHandler(&io_handler);
  inspector->registerProgHandler(&prog_handler);
  inspector->registerProtoHandler(&proto_handler);

  // Should now be registered

  TEST_ASSERT(inspector->getIOHandler() == &io_handler);
  TEST_ASSERT(inspector->getProgHandler() == &prog_handler);
  TEST_ASSERT(inspector->getProtoHandler() == &proto_handler);

  TEST_ASSERT_TRUE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_IO));
  TEST_ASSERT_TRUE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_PROG));
  TEST_ASSERT_TRUE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_PROTO));

  // Clean up

  CDevInspector::destroy();
}

//***************************************************************************
// Description: check handler registration persistence
//***************************************************************************

static void test_dev_inspector_persistence()
{
  CDevInspector *inspector1 = CDevInspector::getInst();
  CIOHandler io_handler;

  // Register one handler

  inspector1->registerIOHandler(&io_handler);
  TEST_ASSERT(inspector1->getIOHandler() == &io_handler);

  // Get instance again - should be same singleton

  CDevInspector *inspector2 = CDevInspector::getInst();
  TEST_ASSERT(inspector1 == inspector2);
  TEST_ASSERT(inspector2->getIOHandler() == &io_handler);

  // Clean up

  CDevInspector::destroy();

  // After destroy, new instance should be fresh

  CDevInspector *inspector3 = CDevInspector::getInst();
  TEST_ASSERT(inspector3->getIOHandler() == nullptr);

  CDevInspector::destroy();
}

//***************************************************************************
// Description: check isRegistered edge cases
//***************************************************************************

static void test_dev_inspector_isregistered()
{
  CDevInspector *inspector = CDevInspector::getInst();

  // Invalid handler type

  TEST_ASSERT_FALSE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_MAX));
  TEST_ASSERT_FALSE(inspector->isRegistered((CDevInspector::EHandlerType)99));

  // Register only IO handler

  CIOHandler io_handler;
  inspector->registerIOHandler(&io_handler);

  TEST_ASSERT_TRUE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_IO));
  TEST_ASSERT_FALSE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_PROG));
  TEST_ASSERT_FALSE(inspector->isRegistered(CDevInspector::HANDLER_TYPE_PROTO));

  // Clean up

  CDevInspector::destroy();
}

extern "C"
{
  int test_dev_inspector()
  {
    UNITY_BEGIN();

    DAWN_RUN_TEST(test_dev_inspector_singleton);
    DAWN_RUN_TEST(test_dev_inspector_registration);
    DAWN_RUN_TEST(test_dev_inspector_persistence);
    DAWN_RUN_TEST(test_dev_inspector_isregistered);

    return UNITY_END();
  }
}
